
#include <iostream>

#include "decoder.h"
#include "io.h"

using namespace std;

int main() {
    io::input_stream is;
    decoder::DecodingError error;

    for (is.next(); !is.complete(); is.next()) {
        if ((error = decoder::decode(is)) == decoder::DecodingError::NONE) continue;

        const char* error_message = decoder::decoding_error_message[static_cast<int>(error)];
        cerr << "\nError: " << error_message << endl;
        cerr << "Decoding failed on: byte = ";
        bit::print_bits(cerr, is.byte());
        cerr << ", position = " << is.pos << "\n";
        return 1;
    }
    return 0;
}
