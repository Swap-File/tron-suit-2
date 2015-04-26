"""
Consistent Overhead Byte Stuffing (COBS)

This version is for Python 3.x.
"""

DSCRC_TABLE = [
    0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65, 
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220, 
    35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98, 
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255, 
    70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7, 
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154, 
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36, 
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185, 
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205, 
    17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80, 
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238, 
    50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115, 
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139, 
    87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22, 
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168, 
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
]



class DecodeError(Exception):
    pass


def _get_buffer_view(in_bytes):
    mv = memoryview(in_bytes)
    if mv.ndim > 1 or mv.itemsize > 1:
        raise BufferError('object must be a single-dimension buffer of bytes.')
    return mv

def getcrc(in_bytes):
    crc = 0
    for i in in_bytes:
        crc = DSCRC_TABLE[crc ^ i]
    return crc

def encode(in_bytes):
    """Encode a string using Consistent Overhead Byte Stuffing (COBS).
    
    Input is any byte string. Output is also a byte string.
    
    Encoding guarantees no zero bytes in the output. The output
    string will be expanded slightly, by a predictable amount.
    
    An empty string is encoded to '\\x01'"""

    if isinstance(in_bytes, str):
        raise TypeError('Unicode-objects must be encoded as bytes first')
    in_bytes.append(getcrc(in_bytes))
    in_bytes_mv = _get_buffer_view(in_bytes)
    final_zero = True
    out_bytes = bytearray()
    idx = 0
    search_start_idx = 0
    for in_char in in_bytes_mv:
        if in_char == b'\x00':
            final_zero = True
            out_bytes.append(idx - search_start_idx + 1)
            out_bytes += in_bytes_mv[search_start_idx:idx]
            search_start_idx = idx + 1
        else:
            if idx - search_start_idx == 0xFD:
                final_zero = False
                out_bytes.append(0xFF)
                out_bytes += in_bytes_mv[search_start_idx:idx+1]
                search_start_idx = idx + 1
        idx += 1
    if idx != search_start_idx or final_zero:
        out_bytes.append(idx - search_start_idx + 1)
        out_bytes += in_bytes_mv[search_start_idx:idx]
    out_bytes.append(0)
    out_bytes.insert(0,0)
    return out_bytes


def decode(in_bytes):
    """Decode a string using Consistent Overhead Byte Stuffing (COBS).
    
    Input should be a byte string that has been COBS encoded. Output
    is also a byte string.
    
    A cobs.DecodeError exception may be raised if the encoded data
    is invalid."""
    if isinstance(in_bytes, str):
        raise TypeError('Unicode-objects are not supported; byte buffer objects only')
    in_bytes_mv = _get_buffer_view(in_bytes)
    out_bytes = bytearray()
    idx = 0

    if len(in_bytes_mv) > 0:
        while True:
            length = ord(in_bytes_mv[idx])
            if length == 0:
                raise DecodeError("zero byte found in input")
            idx += 1
            end = idx + length - 1
            copy_mv = in_bytes_mv[idx:end]
            if b'\x00' in copy_mv:
                raise DecodeError("zero byte found in input")
            out_bytes += copy_mv
            idx = end
            if idx > len(in_bytes_mv):
                raise DecodeError("not enough input bytes for length code")
            if idx < len(in_bytes_mv):
                if length < 0xFF:
                    out_bytes.append(0)
            else:
                break
    return out_bytes
