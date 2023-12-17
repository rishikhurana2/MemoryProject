Compile the program by running:

g++ *.cpp -o memory_driver

Then run the code on one of the provided test files by running the command:

./memory_driver [file_name]

For example, one of the provided test files is L2-test.txt -- so we can run the command "./memory_driver L2-test.txt". The output is the triplet (L1_miss_rate, L2_miss_rate, AAT), where
L1_miss_rate is the miss rate of the L1 cache, L2_miss_rate is the miss rate of the L2 cache, and AAT is the average access time of the memory hierarchy.

If you want to create your own file to test with, the format of each line of each file is

"[read_bit], [write_bit], [mem_addr], [data_to_write]"

where [read_bit] indicates weather or not you want to read from the memory hierarchy at [mem_adr]; [write_bit] indicates if you want to write the specified [data_to_write] at [mem_adr].
Note that if [read_bit] is one, then you must have [write_bit] set to 0 and [data_to_write] set to 0 (because you are reading).
