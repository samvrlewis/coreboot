def parse_section_table_entry(entry):
    
    parts = {"section_type": entry & 0x3,
            "B": (entry & 4) >> 2,
            "C": (entry & 8) >> 3,
            "XN": (entry & 16) >> 4,
            "DOMAIN" : (entry & 0x1E0) >> 5,
            "P" : (entry & 0x200) >> 9,
            "AP" : (entry & 0xC00) >> 10,
            "TEX" : (entry & 0x7000 ) >> 12,
            "APX" : (entry & 0x8000) >> 15,
            "S" : (entry & 0x10000) >> 16,
            "NG" : (entry & 0x20000) >> 17,
            "always_zero" : (entry & 0x40000) >> 18,
            "SBZ" : (entry & 0x80000) >> 19,
            "BASE_ADDRESS" : (entry & 0x7FF00000) >> 20,
    }

    for part in parts:
        print("{}: {}".format(part, parts[part]))



