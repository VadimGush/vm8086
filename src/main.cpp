
#include "instructions/mov.h"
#include "errors.h"
#include "io.h"
#include <iostream>
using namespace std;

int decode(io::input_stream& is) {
    errors::DecodingError error;

    for (is.next(); !is.complete(); is.next()) {
        if ((error = mov::decode(is)) == errors::DecodingError::NONE) continue;

        const char* error_message = errors::decoding_error_message[static_cast<int>(error)];
        cerr << "\nError: " << error_message << endl;
        cerr << "Decoding failed on: byte = ";
        bit::print_bits(cerr, is.byte());
        cerr << ", position = " << is.pos << "\n";
        return 1;
    }
    return 0;
}

int main() {
    io::input_stream is;
    return decode(is);
}
