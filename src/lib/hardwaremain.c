/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */


/*
 * C Bootstrap code for the coreboot
 */

#include <adainit.h>
#include <arch/exception.h>
#include <bootstate.h>
#include <console/console.h>
#include <console/post_codes.h>
#include <commonlib/helpers.h>
#include <cbmem.h>
#include <version.h>
#include <device/device.h>
#include <device/pci.h>
#include <delay.h>
#include <stdlib.h>
#include <boot/tables.h>
#include <program_loading.h>
#if CONFIG(HAVE_ACPI_RESUME)
#include <acpi/acpi.h>
#endif
#include <timer.h>
#include <timestamp.h>
#include <thread.h>

#include <mainboard/ti/beaglebone/sdmmc.h>

static boot_state_t bs_pre_device(void *arg);
static boot_state_t bs_dev_init_chips(void *arg);
static boot_state_t bs_dev_enumerate(void *arg);
static boot_state_t bs_dev_resources(void *arg);
static boot_state_t bs_dev_enable(void *arg);
static boot_state_t bs_dev_init(void *arg);
static boot_state_t bs_post_device(void *arg);
static boot_state_t bs_os_resume_check(void *arg);
static boot_state_t bs_os_resume(void *arg);
static boot_state_t bs_write_tables(void *arg);
static boot_state_t bs_payload_load(void *arg);
static boot_state_t bs_payload_boot(void *arg);

/* The prologue (BS_ON_ENTRY) and epilogue (BS_ON_EXIT) of a state can be
 * blocked from transitioning to the next (state,seq) pair. When the blockers
 * field is 0 a transition may occur. */
struct boot_phase {
	struct boot_state_callback *callbacks;
	int blockers;
};

struct boot_state {
	const char *name;
	boot_state_t id;
	u8 post_code;
	struct boot_phase phases[2];
	boot_state_t (*run_state)(void *arg);
	void *arg;
	int num_samples;
	int complete : 1;
};

#define BS_INIT(state_, run_func_)				\
	{							\
		.name = #state_,				\
		.id = state_,					\
		.post_code = POST_ ## state_,			\
		.phases = { { NULL, 0 }, { NULL, 0 } },		\
		.run_state = run_func_,				\
		.arg = NULL,					\
		.complete = 0,					\
	}
#define BS_INIT_ENTRY(state_, run_func_)	\
	[state_] = BS_INIT(state_, run_func_)

static struct boot_state boot_states[] = {
	BS_INIT_ENTRY(BS_PRE_DEVICE, bs_pre_device),
	BS_INIT_ENTRY(BS_DEV_INIT_CHIPS, bs_dev_init_chips),
	BS_INIT_ENTRY(BS_DEV_ENUMERATE, bs_dev_enumerate),
	BS_INIT_ENTRY(BS_DEV_RESOURCES, bs_dev_resources),
	BS_INIT_ENTRY(BS_DEV_ENABLE, bs_dev_enable),
	BS_INIT_ENTRY(BS_DEV_INIT, bs_dev_init),
	BS_INIT_ENTRY(BS_POST_DEVICE, bs_post_device),
	BS_INIT_ENTRY(BS_OS_RESUME_CHECK, bs_os_resume_check),
	BS_INIT_ENTRY(BS_OS_RESUME, bs_os_resume),
	BS_INIT_ENTRY(BS_WRITE_TABLES, bs_write_tables),
	BS_INIT_ENTRY(BS_PAYLOAD_LOAD, bs_payload_load),
	BS_INIT_ENTRY(BS_PAYLOAD_BOOT, bs_payload_boot),
};

void __weak arch_bootstate_coreboot_exit(void) { }

static boot_state_t bs_pre_device(void *arg)
{
	return BS_DEV_INIT_CHIPS;
}

static boot_state_t bs_dev_init_chips(void *arg)
{
	timestamp_add_now(TS_DEVICE_ENUMERATE);

	/* Initialize chips early, they might disable unused devices. */
	dev_initialize_chips();

	return BS_DEV_ENUMERATE;
}

static boot_state_t bs_dev_enumerate(void *arg)
{
	/* Find the devices we don't have hard coded knowledge about. */
	dev_enumerate();

	return BS_DEV_RESOURCES;
}

static boot_state_t bs_dev_resources(void *arg)
{
	timestamp_add_now(TS_DEVICE_CONFIGURE);

	/* Now compute and assign the bus resources. */
	dev_configure();

	return BS_DEV_ENABLE;
}

static boot_state_t bs_dev_enable(void *arg)
{
	timestamp_add_now(TS_DEVICE_ENABLE);

	/* Now actually enable devices on the bus */
	dev_enable();

	return BS_DEV_INIT;
}

static boot_state_t bs_dev_init(void *arg)
{
	timestamp_add_now(TS_DEVICE_INITIALIZE);

	/* And of course initialize devices on the bus */
	dev_initialize();

	return BS_POST_DEVICE;
}

static boot_state_t bs_post_device(void *arg)
{
	dev_finalize();
	timestamp_add_now(TS_DEVICE_DONE);

	return BS_OS_RESUME_CHECK;
}

static boot_state_t bs_os_resume_check(void *arg)
{
#if CONFIG(HAVE_ACPI_RESUME)
	void *wake_vector;

	wake_vector = acpi_find_wakeup_vector();

	if (wake_vector != NULL) {
		boot_states[BS_OS_RESUME].arg = wake_vector;
		return BS_OS_RESUME;
	}
#endif
	timestamp_add_now(TS_CBMEM_POST);

	return BS_WRITE_TABLES;
}

static boot_state_t bs_os_resume(void *wake_vector)
{
#if CONFIG(HAVE_ACPI_RESUME)
	arch_bootstate_coreboot_exit();
	acpi_resume(wake_vector);
#endif
	return BS_WRITE_TABLES;
}

static boot_state_t bs_write_tables(void *arg)
{
	timestamp_add_now(TS_WRITE_TABLES);

	/* Now that we have collected all of our information
	 * write our configuration tables.
	 */
	write_tables();

	timestamp_add_now(TS_FINALIZE_CHIPS);
	dev_finalize_chips();

	return BS_PAYLOAD_LOAD;
}

static boot_state_t bs_payload_load(void *arg)
{
	payload_load();

	return BS_PAYLOAD_BOOT;
}

static boot_state_t bs_payload_boot(void *arg)
{
	arch_bootstate_coreboot_exit();
	payload_run();

	printk(BIOS_EMERG, "Boot failed\n");
	/* Returning from this state will fail because the following signals
	 * return to a completed state. */
	return BS_PAYLOAD_BOOT;
}

/*
 * Typically a state will take 4 time samples:
 *   1. Before state entry callbacks
 *   2. After state entry callbacks / Before state function.
 *   3. After state function / Before state exit callbacks.
 *   4. After state exit callbacks.
 */
static void bs_sample_time(struct boot_state *state)
{
	static const char *const sample_id[] = { "entry", "run", "exit" };
	static struct mono_time previous_sample;
	struct mono_time this_sample;
	long console;

	if (!CONFIG(HAVE_MONOTONIC_TIMER))
		return;

	console = console_time_get_and_reset();
	timer_monotonic_get(&this_sample);
	state->num_samples++;

	int i = state->num_samples - 2;
	if ((i >= 0) && (i < ARRAY_SIZE(sample_id))) {
		long execution = mono_time_diff_microseconds(&previous_sample, &this_sample);

		/* Report with millisecond precision to reduce log diffs. */
		execution = DIV_ROUND_CLOSEST(execution, USECS_PER_MSEC);
		console = DIV_ROUND_CLOSEST(console, USECS_PER_MSEC);
		if (execution) {
			printk(BIOS_DEBUG, "BS: %s %s times (exec / console): %ld / %ld ms\n",
				state->name, sample_id[i], execution - console, console);
			/* Reset again to ignore printk() time above. */
			console_time_get_and_reset();
		}
	}
	timer_monotonic_get(&previous_sample);
}

#if CONFIG(TIMER_QUEUE)
static void bs_run_timers(int drain)
{
	/* Drain all timer callbacks until none are left, if directed.
	 * Otherwise run the timers only once. */
	do {
		if (!timers_run())
			break;
	} while (drain);
}
#else
static void bs_run_timers(int drain) {}
#endif

static void bs_call_callbacks(struct boot_state *state,
			      boot_state_sequence_t seq)
{
	struct boot_phase *phase = &state->phases[seq];

	while (1) {
		if (phase->callbacks != NULL) {
			struct boot_state_callback *bscb;

			/* Remove the first callback. */
			bscb = phase->callbacks;
			phase->callbacks = bscb->next;
			bscb->next = NULL;

#if CONFIG(DEBUG_BOOT_STATE)
			printk(BIOS_DEBUG, "BS: callback (%p) @ %s.\n",
				bscb, bscb->location);
#endif
			bscb->callback(bscb->arg);
			continue;
		}

		/* All callbacks are complete and there are no blockers for
		 * this state. Therefore, this part of the state is complete. */
		if (!phase->blockers)
			break;

		/* Something is blocking this state from transitioning. As
		 * there are no more callbacks a pending timer needs to be
		 * ran to unblock the state. */
		bs_run_timers(0);
	}
}

/* Keep track of the current state. */
static struct state_tracker {
	boot_state_t state_id;
	boot_state_sequence_t seq;
} current_phase = {
	.state_id = BS_PRE_DEVICE,
	.seq = BS_ON_ENTRY,
};

static void bs_walk_state_machine(void)
{

	while (1) {
		struct boot_state *state;
		boot_state_t next_id;

		state = &boot_states[current_phase.state_id];

		if (state->complete) {
			printk(BIOS_EMERG, "BS: %s state already executed.\n",
			       state->name);
			break;
		}

		if (CONFIG(DEBUG_BOOT_STATE))
			printk(BIOS_DEBUG, "BS: Entering %s state.\n",
				state->name);

		bs_run_timers(0);

		bs_sample_time(state);

		bs_call_callbacks(state, current_phase.seq);
		/* Update the current sequence so that any calls to block the
		 * current state from the run_state() function will place a
		 * block on the correct phase. */
		current_phase.seq = BS_ON_EXIT;

		bs_sample_time(state);

		post_code(state->post_code);

		next_id = state->run_state(state->arg);

		if (CONFIG(DEBUG_BOOT_STATE))
			printk(BIOS_DEBUG, "BS: Exiting %s state.\n",
			state->name);

		bs_sample_time(state);

		bs_call_callbacks(state, current_phase.seq);

		if (CONFIG(DEBUG_BOOT_STATE))
			printk(BIOS_DEBUG,
				"----------------------------------------\n");

		/* Update the current phase with new state id and sequence. */
		current_phase.state_id = next_id;
		current_phase.seq = BS_ON_ENTRY;

		bs_sample_time(state);

		state->complete = 1;
	}
}

static int boot_state_sched_callback(struct boot_state *state,
				     struct boot_state_callback *bscb,
				     boot_state_sequence_t seq)
{
	if (state->complete) {
		printk(BIOS_WARNING,
		       "Tried to schedule callback on completed state %s.\n",
		       state->name);

		return -1;
	}

	bscb->next = state->phases[seq].callbacks;
	state->phases[seq].callbacks = bscb;

	return 0;
}

int boot_state_sched_on_entry(struct boot_state_callback *bscb,
			      boot_state_t state_id)
{
	struct boot_state *state = &boot_states[state_id];

	return boot_state_sched_callback(state, bscb, BS_ON_ENTRY);
}

int boot_state_sched_on_exit(struct boot_state_callback *bscb,
			     boot_state_t state_id)
{
	struct boot_state *state = &boot_states[state_id];

	return boot_state_sched_callback(state, bscb, BS_ON_EXIT);
}

static void boot_state_schedule_static_entries(void)
{
	extern struct boot_state_init_entry *_bs_init_begin[];
	struct boot_state_init_entry **slot;

	for (slot = &_bs_init_begin[0]; *slot != NULL; slot++) {
		struct boot_state_init_entry *cur = *slot;

		if (cur->when == BS_ON_ENTRY)
			boot_state_sched_on_entry(&cur->bscb, cur->state);
		else
			boot_state_sched_on_exit(&cur->bscb, cur->state);
	}
}


#include <commonlib/storage.h>
#define uart_putf(X...) printk(BIOS_INFO, "ramstage: " X)

/**
 * Write a uint32_t value to a memory address
 */
static inline void write32(uint32_t address, uint32_t value)
{
	REG(address)= value;
}

/**
 * Read an uint32_t from a memory address
 */
static inline uint32_t read32(uint32_t address)
{
	return REG(address);
}

/**
 * Set a 32 bits value depending on a mask
 */
static inline void set32(uint32_t address, uint32_t mask, uint32_t value)
{
	uint32_t val;
	val= read32(address);
	val&= ~(mask); /* clear the bits */
	val|= (value & mask); /* apply the value using the mask */
	write32(address, val);
}

static int send_cmd_base(uint32_t command, uint32_t arg, uint32_t resp_type)
{
		int count= 0;

	uart_putf("send cmdbase %d %d %d\n", command, arg, resp_type);
	/* Read current interrupt status and fail it an interrupt is already asserted */
	if ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0xffffu)) {
		printk(BIOS_DEBUG,"%s, interrupt already raised stat  %08x\n", __FUNCTION__,
				read32(MMCHS0_REG_BASE + MMCHS_SD_STAT));
		return 1;
	}



	/* Set arguments */
	write32(MMCHS0_REG_BASE + MMCHS_SD_ARG, arg);
	/* Set command */
	set32(MMCHS0_REG_BASE + MMCHS_SD_CMD, MMCHS_SD_CMD_MASK, command);

	/* Wait for completion */
	while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0xffffu) == 0x0) {
		count++;
	}

	if (read32(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0x8000) {
		printk(BIOS_DEBUG,"%s, error stat  %08x\n", __FUNCTION__,
				read32(MMCHS0_REG_BASE + MMCHS_SD_STAT));
		set32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_STAT_ERROR_MASK,
				0xffffffffu);	// clear errors
		// We currently only support 2.0, not responding to
		return 1;
	}

	if (resp_type == CARD_RSP_R1b) {
		/*
		 * Command with busy repsonse *CAN* also set the TC bit if they exit busy
		 */
		while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT)
				& MMCHS_SD_IE_TC_ENABLE_ENABLE) == 0) {
			count++;
		}
		write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_TC_ENABLE_CLEAR);
	}

	/* clear the cc status */
	write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_CC_ENABLE_CLEAR);
	return 0;
}

static int send_cmd(struct sd_mmc_ctrlr *ctrlr, struct mmc_command *cmd, struct mmc_data *data)
{

uint32_t count;
	uint32_t value;
	uart_putf("send cmd %d %d %d\n", cmd->cmdidx, cmd->cmdarg, cmd->resp_type);
	if (cmd->cmdidx == 16)
	{
		return 0;
	}

	int ret = 0;

	if (cmd->cmdidx == 17)
	{
		set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_BRR_ENABLE,
			MMCHS_SD_IE_BRR_ENABLE_ENABLE);

		set32(MMCHS0_REG_BASE + MMCHS_SD_BLK, MMCHS_SD_BLK_BLEN, 512);

		if (send_cmd_base(MMCHS_SD_CMD_CMD17 /* read single block */
		| MMCHS_SD_CMD_DP_DATA /* Command with data transfer */
		| MMCHS_SD_CMD_RSP_TYPE_48B /* type (R1) */
		| MMCHS_SD_CMD_MSBS_SINGLE /* single block */
		| MMCHS_SD_CMD_DDIR_READ /* read data from card */
		, cmd->cmdarg, cmd->resp_type)) {
			return 1;
		}

		while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT)
			& MMCHS_SD_IE_BRR_ENABLE_ENABLE) == 0) {
			count++;
		}

		if (!(read32(MMCHS0_REG_BASE + MMCHS_SD_PSTATE) & MMCHS_SD_PSTATE_BRE_EN)) {
			return 1; /* We are not allowed to read data from the data buffer */
		}

		for (count= 0; count < 512; count+= 4) {
			value= read32(MMCHS0_REG_BASE + MMCHS_SD_DATA);
			data->dest[count]= *((char*) &value);
			data->dest[count + 1]= *((char*) &value + 1);
			data->dest[count + 2]= *((char*) &value + 2);
			data->dest[count + 3]= *((char*) &value + 3);
		}

		while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT)
			& MMCHS_SD_IE_TC_ENABLE_ENABLE) == 0) {
			count++;
		}
		write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_TC_ENABLE_CLEAR);

		/* clear and disable the bbr interrupt */
		write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_BRR_ENABLE_CLEAR);
		set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_BRR_ENABLE,
				MMCHS_SD_IE_BRR_ENABLE_DISABLE);
		return 0;

	} else {
		uint32_t command = cmd->cmdidx << 24;

		switch (cmd->resp_type) {
			case CARD_RSP_R1b:
				command |= MMCHS_SD_CMD_RSP_TYPE_48B_BUSY;
				break;
			case CARD_RSP_R1:
			case CARD_RSP_R3:
				command |= MMCHS_SD_CMD_RSP_TYPE_48B;
				break;
			case CARD_RSP_R2:
				command |= MMCHS_SD_CMD_RSP_TYPE_136B;
				break;
			case CARD_RSP_NONE:
				command |= MMCHS_SD_CMD_RSP_TYPE_NO_RESP;
				break;
			default:
				return 1;
		}
		ret = send_cmd_base(command, cmd->cmdarg, cmd->resp_type);

		if (ret)
		{
			return ret;
		}
	}
	

	/* copy response into cmd->resp	 */
	switch (cmd->resp_type) {
		case CARD_RSP_R1:
		case CARD_RSP_R1b:
		case CARD_RSP_R3:
			cmd->response[0] = read32(MMCHS0_REG_BASE + MMCHS_SD_RSP10);
			break;
		case CARD_RSP_R2:
			cmd->response[0] = read32(MMCHS0_REG_BASE + MMCHS_SD_RSP10);
			cmd->response[1] = read32(MMCHS0_REG_BASE + MMCHS_SD_RSP32);
			cmd->response[2] = read32(MMCHS0_REG_BASE + MMCHS_SD_RSP54);
			cmd->response[3] = read32(MMCHS0_REG_BASE + MMCHS_SD_RSP76);
			break;
		case CARD_RSP_NONE:
			break;
		default:
			printk(BIOS_DEBUG, "Unknown case\n");
			return 1;
	}


	return 0;
}

static void set_ios(struct sd_mmc_ctrlr *ctrlr)
{

	/*
	#define SET_BUS_WIDTH(ctrlr, width)		\
	do {					\
		ctrlr->bus_width = width;	\
		ctrlr->set_ios(ctrlr);		\
	} while (0)

	#define SET_CLOCK(ctrlr, clock_hz)		\
		do {					\
			ctrlr->request_hz = clock_hz;	\
			ctrlr->set_ios(ctrlr);		\
		} while (0)

	#define SET_TIMING(ctrlr, timing_value)		\
			ctrlr->timing = timing_value;	\
			ctrlr->set_ios(ctrlr);		\
	*/

	// need to set the bus width, timing and clock

	/*
	width is 1,4,8

	#define CLOCK_KHZ		1000
#define CLOCK_MHZ		(1000 * CLOCK_KHZ)
#define CLOCK_20MHZ		(20 * CLOCK_MHZ)
#define CLOCK_25MHZ		(25 * CLOCK_MHZ)
#define CLOCK_26MHZ		(26 * CLOCK_MHZ)
#define CLOCK_50MHZ		(50 * CLOCK_MHZ)
#define CLOCK_52MHZ		(52 * CLOCK_MHZ)
#define CLOCK_200MHZ		(200 * CLOCK_MHZ)


	#define BUS_TIMING_LEGACY	0
#define BUS_TIMING_MMC_HS	1
#define BUS_TIMING_SD_HS	2
#define BUS_TIMING_UHS_SDR12	3
#define BUS_TIMING_UHS_SDR25	4
#define BUS_TIMING_UHS_SDR50	5
#define BUS_TIMING_UHS_SDR104	6
#define BUS_TIMING_UHS_DDR50	7
#define BUS_TIMING_MMC_DDR52	8
#define BUS_TIMING_MMC_HS200	9
#define BUS_TIMING_MMC_HS400	10
#define BUS_TIMING_MMC_HS400ES	11
*/


	printk(BIOS_DEBUG, "%s: called, bus_width: %x, clock: %d -> %d\n", __func__,
		  ctrlr->bus_width, ctrlr->request_hz, ctrlr->timing);
}

static void init_sd(void)
{
	struct storage_media media;
	struct sd_mmc_ctrlr mmc_ctrlr;
	uint8_t buffer[1024];


	memset(&mmc_ctrlr, 0, sizeof(mmc_ctrlr));
	memset(&buffer, 0, sizeof(buffer));
	mmc_ctrlr.send_cmd = &send_cmd;
	mmc_ctrlr.set_ios = &set_ios;

	mmc_ctrlr.voltages = MMC_VDD_30_31;
	mmc_ctrlr.b_max = 1;

	

	media.ctrlr = &mmc_ctrlr;

	printk(BIOS_DEBUG, "pre storage\n");

	storage_setup_media(&media, &mmc_ctrlr);

	printk(BIOS_DEBUG, "post storage\n");

	storage_display_setup(&media);

	storage_block_read(&media, 0, 2, &buffer);

	for (int i=0; i<1024; i+=2)
	{

		if (i%16==0)
		{
			printk(BIOS_DEBUG, "\n%08x: ", i);
		}
		printk(BIOS_DEBUG, "%02x%02x ", buffer[i], buffer[i+1]);
	}

	//storage_block_read(&media, 0, 1, &buffer);

	//storage_block_read(&media, 0, 1, &buffer);

	

	//	printk(BIOS_DEBUG, "post storage\n");
}

void main(void)
{
	/*
	 * We can generally jump between C and Ada code back and forth
	 * without trouble. But since we don't have an Ada main() we
	 * have to do some Ada package initializations that GNAT would
	 * do there. This has to be done before calling any Ada code.
	 *
	 * The package initializations should not have any dependen-
	 * cies on C code. So we can call them here early, and don't
	 * have to worry at which point we can start to use Ada.
	 */
	ramstage_adainit();

	/* TODO: Understand why this is here and move to arch/platform code. */
	/* For MMIO UART this needs to be called before any other printk. */
	if (CONFIG(ARCH_X86))
		init_timer();

	/* console_init() MUST PRECEDE ALL printk()! Additionally, ensure
	 * it is the very first thing done in ramstage.*/
	console_init();
	post_code(POST_CONSOLE_READY);

	exception_init();

	/*
	 * CBMEM needs to be recovered because timestamps, ACPI, etc rely on
	 * the cbmem infrastructure being around. Explicitly recover it.
	 */
	cbmem_initialize();

	timestamp_add_now(TS_START_RAMSTAGE);
	post_code(POST_ENTRY_RAMSTAGE);

	init_sd();

	return;

	/* Handoff sleep type from romstage. */
#if CONFIG(HAVE_ACPI_RESUME)
	acpi_is_wakeup();
#endif
	threads_initialize();

	/* Schedule the static boot state entries. */
	boot_state_schedule_static_entries();

	bs_walk_state_machine();

	die("Boot state machine failure.\n");
}


int boot_state_block(boot_state_t state, boot_state_sequence_t seq)
{
	struct boot_phase *bp;

	/* Blocking a previously ran state is not appropriate. */
	if (current_phase.state_id > state ||
	    (current_phase.state_id == state && current_phase.seq > seq)) {
		printk(BIOS_WARNING,
		       "BS: Completed state (%d, %d) block attempted.\n",
		       state, seq);
		return -1;
	}

	bp = &boot_states[state].phases[seq];
	bp->blockers++;

	return 0;
}

int boot_state_unblock(boot_state_t state, boot_state_sequence_t seq)
{
	struct boot_phase *bp;

	/* Blocking a previously ran state is not appropriate. */
	if (current_phase.state_id > state ||
	    (current_phase.state_id == state && current_phase.seq > seq)) {
		printk(BIOS_WARNING,
		       "BS: Completed state (%d, %d) unblock attempted.\n",
		       state, seq);
		return -1;
	}

	bp = &boot_states[state].phases[seq];

	if (bp->blockers == 0) {
		printk(BIOS_WARNING,
		       "BS: Unblock attempted on non-blocked state (%d, %d).\n",
		       state, seq);
		return -1;
	}

	bp->blockers--;

	return 0;
}

void boot_state_current_block(void)
{
	boot_state_block(current_phase.state_id, current_phase.seq);
}

void boot_state_current_unblock(void)
{
	boot_state_unblock(current_phase.state_id, current_phase.seq);
}
