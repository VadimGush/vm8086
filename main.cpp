#define READ_BUFFER_SIZE 1024

#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

struct data_block {
    char buffer[READ_BUFFER_SIZE];
    // TODO: this will result in stack overflow 
    unique_ptr<data_block> next;
};

// Reads binary data from the standart input
vector<char> read_data() {
    data_block block = { .next = nullptr };
    data_block* current_block = &block;

    // Read input in blocks of binary data
    size_t total_bytes = 0;
    while (true) {
        const size_t bytes = read(0, current_block->buffer, READ_BUFFER_SIZE);
        total_bytes += bytes;

        if (bytes == READ_BUFFER_SIZE) {
            current_block->next = make_unique<data_block>();
            current_block = current_block->next.get();
        } else {
            break;
        }
    }

    // Convert blocks of data into one big buffer
    vector<char> data(total_bytes);
    current_block = &block;
    size_t block_id = 0;
    while (current_block != nullptr) {
        const size_t bytes_to_copy = min(total_bytes, (size_t) READ_BUFFER_SIZE);
        memcpy(&data[block_id * READ_BUFFER_SIZE], current_block->buffer, bytes_to_copy);
        total_bytes -= bytes_to_copy;

        current_block = current_block->next.get();
        block_id += 1;
    }
    return data;
}

int main() {

    vector<char> data = read_data();
    for (const char l : data) {
        cout << l;
    }
    cout << endl;

    return 0;
}
