#include "PREDEF"
	
OUTPUT_FORMAT("elf32-sparc", "elf32-sparc", "elf32-sparc")
OUTPUT_ARCH(sparc)
ENTRY(_start)
SECTIONS
{
        /DISCARD/ : {
        *(.exit.text)
        *(.exit.data)
        *(.exitcall.exit)
        *(.comment)
        *(.debug_aranges)
        *(.debug_pubnames)
        *(.debug_abbrev)
        *(.debug_info)
        *(.debug_line)
        *(.debug_frame)
        *(.debug_str)
        *(.debug_ranges)
	*(.debug_loc)
	*(.note.GNU-stack)
        }
        . = LEONSETUP_MEM_BASEADDR /*0x40000000*/;
        .stage2 : {
                _start = .;
                PROMSTAGE2(.img.main.text)
                PROMSTAGE2
                . = ALIGN(0x2000);
                _bootloader_ph = .;
                PGTO
        }
	
        . = ALIGN(0x4000);
        .vmlinux : {
                IMAGE 
        }
}
