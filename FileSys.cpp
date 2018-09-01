// CPSC 3500: File System
// Implements the file system commands that are available to the shell.
// Jeremiah Kalmus
// P1

#include <cstring>
#include <iostream>
#include <stdio.h>
#include <string.h>
using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"

// mounts the file system
void FileSys::mount() {
  bfs.mount();
  curr_dir = 1;
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
}

// make a directory
void FileSys::mkdir(const char *name) 
{
	
	struct dirblock_t curDir;
	
	bfs.read_block(curr_dir, (void*)&curDir);
	
	// check if max number of directory entries is reached
	if (curDir.num_entries == MAX_DIR_ENTRIES) {
		cerr << "Directory is full" << endl;
		return;
	}
	
	// check if file name is too long
	if (strlen(name) > MAX_FNAME_SIZE) {
		cerr << "File name is too long" << endl;
		return;
	}
	
	// check if desired file already exists in current directory
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (strncmp(curDir.dir_entries[i].name,name,MAX_FNAME_SIZE) == 0) {
			cerr << "File exists" << endl;
			return;
		}
	}
	
	//check if disk is full
	int new_dir = bfs.get_free_block();
	if (new_dir == 0) {
		cerr << "Disk is full" << endl;
		return;
	}
	
	// make the directory
	struct dirblock_t newDir;
	
	newDir.magic = DIR_MAGIC_NUM;
	newDir.num_entries = 0;
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		newDir.dir_entries[i].block_num = 0;
	}
	bfs.write_block(new_dir, (void*)&newDir);
	link(curDir, new_dir, name);
}

// switch to a directory
void FileSys::cd(const char *name) {
	struct dirblock_t curDir;
	bfs.read_block(curr_dir, (void*)&curDir);
	
	// find desired file
	int correct_entry = -1;
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (strncmp(curDir.dir_entries[i].name,name,MAX_FNAME_SIZE) == 0) {
			correct_entry = i;
		}
	}
	
	// check if file exists
	if (correct_entry == -1) {
		cerr << "File does not exist" << endl;
		return;
	}
	
	// check if file is a directory
	if (!is_directory(curDir.dir_entries[correct_entry].block_num)) {
		cerr << "File is not a directory" << endl;
		return;
	}
	
	// switch to desired directory
	curr_dir = curDir.dir_entries[correct_entry].block_num;
}

// switch to home directory
void FileSys::home() {
	curr_dir = 1;
}

// remove a directory
void FileSys::rmdir(const char *name) 
{
	struct dirblock_t curDir;
	
	bfs.read_block(curr_dir, (void*)&curDir);
	
	// find desired file
	int correct_entry = -1;
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (strncmp(curDir.dir_entries[i].name,name,MAX_FNAME_SIZE) == 0) {
			correct_entry = i;
		}
	}
	
	// check if file exists
	if (correct_entry == -1) {
		cerr << "File does not exist" << endl;
		return;
	}
	
	// check if file is a directory
	if (!is_directory(curDir.dir_entries[correct_entry].block_num)) {
		cerr << "File is not a directory" << endl;
		return;
	}
	
	// read in directory block
	struct dirblock_t delDir;
	bfs.read_block(curDir.dir_entries[correct_entry].block_num,(void*)&delDir);

	// check that directory is empty
	if(delDir.num_entries != 0) {
		cerr << "Directory is not empty" << endl;
		return;
	}
	
	// remove directory
	bfs.reclaim_block(curDir.dir_entries[correct_entry].block_num);
	
	curDir.num_entries--;
	curDir.dir_entries[correct_entry].block_num = 0;
	char name_erase[10];
	name_erase[0] = ' ';
	strncpy(curDir.dir_entries[correct_entry].name, name_erase,MAX_FNAME_SIZE);
	
	bfs.write_block(curr_dir, (void*)&curDir);
}

// list contents of current directory
void FileSys::ls() 
{
	struct dirblock_t curDir;
	
	bfs.read_block(curr_dir, (void*)&curDir);
	
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (curDir.dir_entries[i].block_num != 0) {
			cout << curDir.dir_entries[i].name;
			
			// if file is a directoty, add '/' to end of name
			if (is_directory(curDir.dir_entries[i].block_num)) {
				cout <<"/";
			}
			cout << endl;
		}
	}
}

// create empty data file
void FileSys::create(const char *name) 
{
	struct dirblock_t curDir;
	
	bfs.read_block(curr_dir, (void*)&curDir);
	
	// check if max number of directory entries is reached
	if (curDir.num_entries == MAX_DIR_ENTRIES) {
		cerr << "Directory is full" << endl;
		return;
	}
	
	// check if file name is too long
	if (strlen(name) > MAX_FNAME_SIZE) {
		cerr << "File name is too long" << endl;
		return;
	}
	
	// check if desired file already exists in current directory
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (strncmp(curDir.dir_entries[i].name,name,MAX_FNAME_SIZE)==0) {
			cerr << "File exists" << endl;
			return;
		}
	}
	
	//check for full disk
	int newNode = bfs.get_free_block();	
	if (newNode == 0) {
		cerr << "Disk is full" << endl;
		return;
	}
	
	// write new i-node to disk
	struct inode_t newiBlock;
	newiBlock.magic = INODE_MAGIC_NUM;
	newiBlock.size = 0;
	for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
		newiBlock.blocks[i] = 0;
	}
	bfs.write_block(newNode, (void*)&newiBlock);
	link(curDir, newNode, name);
}


// append data to a data file
void FileSys::append(const char *name, const char *data) 
{
	struct dirblock_t curDir;
	
	bfs.read_block(curr_dir, (void*)&curDir);
	
	// find desired file
	int correct_entry = -1;
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (strncmp(curDir.dir_entries[i].name,name,MAX_FNAME_SIZE) == 0) {
			correct_entry = i;
		}
	}
	
	// check if file exists
	if (correct_entry == -1) {
		cerr << "File does not exist" << endl;
		return;
	}
	
	// check if file is a directory
	if (is_directory(curDir.dir_entries[correct_entry].block_num)) {
		cerr << "File is a directory" << endl;
		return;
	}
	
	// check if append exceeds max file size
	struct inode_t curiBlock;
	bfs.read_block(curDir.dir_entries[correct_entry].block_num,(void*)&curiBlock);
	if ((curiBlock.size + strlen(data)) > MAX_FILE_SIZE) {
		cerr << "Append exceeds maximum file size" << endl;
		return;
	}
	
	// find number of blocks needed
	int datablock_index = curiBlock.size/BLOCK_SIZE;
	int partial = curiBlock.size%BLOCK_SIZE;
	int free_space = BLOCK_SIZE - partial;
	int data_size = strlen(data);
	int blocks_needed;

	if (data_size <= partial) {
		blocks_needed = 0;
	} else {
		blocks_needed = (data_size - (BLOCK_SIZE - partial)) / BLOCK_SIZE;
		if ((data_size - (BLOCK_SIZE - partial)) % BLOCK_SIZE != 0)
			blocks_needed++;
	}

	// attempt to grab needed blocks, and check if disk is full
	int * temp_block_arr;
	bool full_flag = false;;
	temp_block_arr = new int[blocks_needed];
	for (int i = 0; i < blocks_needed; i++) {
		temp_block_arr[i] = bfs.get_free_block();
		if (temp_block_arr[i] == 0) {
			full_flag=true;
		}
	}
	
	// return blocks and report error if disk full
	for (int i = 0; i < blocks_needed; i++) {
		bfs.reclaim_block(temp_block_arr[i]);
	}
	delete[] temp_block_arr;
	if (full_flag) {
		cerr << "Disk is full" << endl;
		return;
	}
	
	// prepare to write data
	int data_index = 0;
	int curr_datablock_index = (curiBlock.size/BLOCK_SIZE);
	if (curiBlock.blocks[curr_datablock_index] == 0) {
		curiBlock.blocks[curr_datablock_index] = bfs.get_free_block();
	}
	
	// fill the partially filled block (if available)
	struct datablock_t curr_datablock;
	bfs.read_block(curiBlock.blocks[curr_datablock_index],(void*)&curr_datablock);
	if (free_space > strlen(data)) {
		for (int i = 0; i < strlen(data); i++) {
			curr_datablock.data[BLOCK_SIZE-free_space+i] = data[data_index];
			data_index++;
		}
		data_index = strlen(data);
	} 
	else if (free_space > 0) {
		for (int i = BLOCK_SIZE-free_space; i < BLOCK_SIZE; i++) {
			curr_datablock.data[i] = data[data_index];
			data_index++;
		}
	}
	bfs.write_block(curiBlock.blocks[curr_datablock_index],(void*)&curr_datablock);
	
	// put remaining data in new blocks.
	while (data_index < strlen(data)) {
		curr_datablock_index++;
		curiBlock.blocks[curr_datablock_index] = bfs.get_free_block();
		bfs.read_block(curiBlock.blocks[curr_datablock_index],(void*)&curr_datablock);
		if (strlen(data)-data_index < BLOCK_SIZE) {
			for (int i = 0; i < strlen(data)-data_index; i++) {
				curr_datablock.data[i] = data[data_index];
				data_index++;
			}
		} 
		else {
			for (int i = 0; i < BLOCK_SIZE; i++) {
				curr_datablock.data[i] = data[data_index];
				data_index++;
			}
		}
		bfs.write_block(curiBlock.blocks[curr_datablock_index],(void*)&curr_datablock);
	}
	
	// update i-node information
	curiBlock.size += strlen(data);
	bfs.write_block(curDir.dir_entries[correct_entry].block_num,(void*)&curiBlock);


}

// display the contents of a data file
void FileSys::cat(const char *name) 
{
	struct dirblock_t curDir;
	
	bfs.read_block(curr_dir, (void*)&curDir);
	
	// find desired file
	int correct_entry = -1;
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (strncmp(curDir.dir_entries[i].name,name,MAX_FNAME_SIZE) == 0) {
			correct_entry = i;
		}
	}
	
	// check if file exists
	if (correct_entry == -1) {
		cerr << "File does not exist" << endl;
		return;
	}
	
	// check if file is a directory
	if (is_directory(curDir.dir_entries[correct_entry].block_num)) {
		cerr << "File is a directory" << endl;
		return;
	}
	
	// read in i-node block
	struct inode_t curiBlock;
	bfs.read_block(curDir.dir_entries[correct_entry].block_num,(void*)&curiBlock);

	// find file length and prepare to read
	int n = curiBlock.size;
	int data_location = 0;
	int to_read = n/BLOCK_SIZE;
	int leftover = n%BLOCK_SIZE;
	if (leftover != 0) {
		to_read++;
	}
	struct datablock_t curdBlock;
		
	// read desired amount of data
	for (int i = 0; i < to_read; i++) {
		bfs.read_block(curiBlock.blocks[i], (void*)&curdBlock);
		if ((n-data_location) > BLOCK_SIZE) {
			for (int j = 0; j < BLOCK_SIZE; j++) {
				cout << curdBlock.data[j];
			}
		} else {
			for (int j = 0; j < n-data_location; j++) {
				cout << curdBlock.data[j];
			}
		}
	}
	cout << endl;
}

// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n) 
{
	struct dirblock_t curDir;
	
	bfs.read_block(curr_dir, (void*)&curDir);
	
	// find desired file
	int correct_entry = -1;
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (strncmp(curDir.dir_entries[i].name,name,MAX_FNAME_SIZE) == 0) {
			correct_entry = i;
		}
	}
	
	// check if file exists
	if (correct_entry == -1) {
		cerr << "File does not exist" << endl;
		return;
	}
	
	// check if file is a directory
	if (is_directory(curDir.dir_entries[correct_entry].block_num)) {
		cerr << "File is a directory" << endl;
		return;
	}
	
	// read in i-node block
	struct inode_t curiBlock;
	bfs.read_block(curDir.dir_entries[correct_entry].block_num,(void*)&curiBlock);

	// prepare to read
	if (n > curiBlock.size) {
		n = curiBlock.size;
	}
	int data_location = 0;
	int to_read = n / BLOCK_SIZE;
	int leftover = n % BLOCK_SIZE;
	if (leftover != 0) {
		to_read++;
	}
	struct datablock_t curdBlock;
		
	// read desired amount of data
	for (int i = 0; i < to_read; i++) {
		bfs.read_block(curiBlock.blocks[i], (void*)&curdBlock);
		if ((n-data_location) > BLOCK_SIZE) {
			for (int j = 0; j < BLOCK_SIZE; j++) {
				cout << curdBlock.data[j];
			}
		} else {
			for (int j = 0; j < n-data_location; j++) {
				cout << curdBlock.data[j];
			}
		}
	}
	cout << endl;
}

// delete a data file
void FileSys::rm(const char *name) 
{
	struct dirblock_t curDir;
	
	bfs.read_block(curr_dir, (void*)&curDir);
	
	// find desired file
	int correct_entry = -1;
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (strncmp(curDir.dir_entries[i].name,name,MAX_FNAME_SIZE) == 0) {
			correct_entry = i;
		}
	}
	
	// check if file exists
	if (correct_entry == -1) {
		cerr << "File does not exist" << endl;
		return;
	}
	
	// check if file is a directory
	if (is_directory(curDir.dir_entries[correct_entry].block_num)) {
		cerr << "File is a directory" << endl;
		return;
	}
	
	// read in i-node block
	struct inode_t deliBlock;
	bfs.read_block(curDir.dir_entries[correct_entry].block_num,(void*)&deliBlock);

	// erase and reclaim all data blocks
	struct datablock_t deldBlock;
	char data_erase[10];
	data_erase[0]	= ' ';

	for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
		if (deliBlock.blocks[i] != 0) {
			bfs.read_block(deliBlock.blocks[i], (void*)&deldBlock);
			strncpy(deldBlock.data, data_erase, strlen(deldBlock.data));
			bfs.write_block(deliBlock.blocks[i], (void*)&deldBlock);
			bfs.reclaim_block(deliBlock.blocks[i]);
		}
	}

	// remove i-node and update parent directory
	bfs.reclaim_block(curDir.dir_entries[correct_entry].block_num);
	
	curDir.num_entries--;
	curDir.dir_entries[correct_entry].block_num = 0;
	char name_erase[10];
	name_erase[0] = ' ';
	strncpy(curDir.dir_entries[correct_entry].name, name_erase,MAX_FNAME_SIZE);
	
	bfs.write_block(curr_dir, (void*)&curDir);
}

// display stats about file or directory
void FileSys::stat(const char *name) 
{
	struct dirblock_t curDir;
	
	bfs.read_block(curr_dir, (void*)&curDir);
	
		// find desired file
	int correct_entry = -1;
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (strncmp(curDir.dir_entries[i].name,name,MAX_FNAME_SIZE) == 0) {
			correct_entry = i;
		}
	}
	
	// check if file exists
	if (correct_entry == -1) {
		cerr << "File does not exist" << endl;
		return;
	}
	
	// check if file is a directory or not
	if (is_directory(curDir.dir_entries[correct_entry].block_num)) {
		// print stats for directory
		cout << "Directory name: " << curDir.dir_entries[correct_entry].name
				 << "/" << endl
				 << "Directory block: "
				 << curDir.dir_entries[correct_entry].block_num << endl;
	} 
	else {
		// gather inode stats
		struct inode_t stat_inodeblock;
		bfs.read_block(curDir.dir_entries[correct_entry].block_num ,(void*)&stat_inodeblock);
		int num_used_blocks = 1;
		
		for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
			if (stat_inodeblock.blocks[i] != 0) {
				num_used_blocks++;
			}
		}
		// print stats for inode
		cout << "Inode block: "
			 << curDir.dir_entries[correct_entry].block_num << endl
			 << "Bytes in file: " << stat_inodeblock.size << endl
			 << "Number of blocks: " << num_used_blocks << endl
			 << "First block: " << stat_inodeblock.blocks[0] << endl;
	}
}

// checks if a block corresponds to a directory
bool FileSys::is_directory(const short block_num) 
{
	struct dirblock_t testblock;
	
	bfs.read_block(block_num, (void*)&testblock);
	
	if (testblock.magic == DIR_MAGIC_NUM) {
		return true;
	} 
	else {
		return false;
	}
}

// link new i-node or directory to current directory
void FileSys::link(dirblock_t curDir, const short bNum, const char *new_name)
{
	for (int i = 0; i < MAX_DIR_ENTRIES; i++){
		if (curDir.dir_entries[i].block_num == 0) {
			curDir.dir_entries[i].block_num = bNum;
			strncpy(curDir.dir_entries[i].name, new_name,MAX_FNAME_SIZE);
			curDir.num_entries++;
			
			bfs.write_block(curr_dir, (void*)&curDir);
			return;
		}
	}
}