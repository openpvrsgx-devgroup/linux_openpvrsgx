This tools can be used to analyse ABE DUMP file. It is base on 3.0 Kernel.
Since 3.0 kernel ABE HAL is using only bytes OFFSET for memory location.

How to use:
- Make a abe_dump on the TARGET
- Getback the outputs files from the TARGET
    * Copy dmem.txt, smem.txt, cmem.txt, aess.txt on the folder 
- Compile the tool.
- Run ./abe_check to get the result.

TODO:
  - Adapt buffer scaling factor according to IO descriptor
  - List only the buffer associated to the port that are enable.
  - Rework the code to have clean library.
