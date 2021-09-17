# BIOS

Ref: _System_BIOS_for_IBM_PC_XT_AT_Computers_and_Compatibles.pdf_

### Where does BIOS store data
> BIOS Data Area - At POST, the BIOS stores a set of data definitions in system RAM in absolute memory location 400h-500h. In the process of executing device service routines, the BIOS refers to and updates this data. For example, the BIOS updates 40:50h, the location of the cursor on the video page, each time the BIOS routine "Set Cursor Position" is executed.