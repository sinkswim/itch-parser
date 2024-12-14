import struct
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from typing import Dict, List, Any
from datetime import datetime, timedelta
import argparse
import time

# Define here the TotalView ITCH 5.0 messages bin file to process with this script
# These files are quite large, eg. over 10GB, and are thus omitted in this repo.
# Find one here: https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/
BIN_FILE = "../01302019.NASDAQ_ITCH50"

# The reference TotalView ITCH 5.0 protocol breakdown, used to parse the bin file
XML_FILE = "../nasdaq_totalview_itch.xml"

class ITCHMessageParser:
    def __init__(self, xml_spec_path: str):
        self.message_specs = {}
        self.parse_xml_spec(xml_spec_path)
        
    def parse_xml_spec(self, xml_path: str):
        """Parse the XML specification file to build message type definitions."""
        tree = ET.parse(xml_path)
        root = tree.getroot()
        
        # Parse all struct definitions
        for struct in root.findall(".//Struct"):
            msg_len = int(struct.get('len'))
            msg_id = struct.get('id')
            if msg_id:  # Only process message types, not utility structs
                fields = []
                for field in struct.findall('Field'):
                    fields.append({
                        'name': field.get('name'),
                        'offset': int(field.get('offset')),
                        'length': int(field.get('len')),
                        'type': field.get('type')
                    })
                self.message_specs[msg_id] = {
                    'length': msg_len,
                    'fields': fields
                }

    def parse_binary_value(self, data: bytes, field_type: str) -> Any:
        """Convert binary data to Python value based on field type."""
        if field_type.startswith('char'):
            return data.decode('ascii').strip()
        elif field_type == 'price_4_t':
            return struct.unpack('>I', data)[0] / 10000.0
        elif field_type == 'price_8_t':
            return struct.unpack('>Q', data)[0] / 100000000.0
        elif field_type == 'u8_t':
            return struct.unpack('>B', data)[0]
        elif field_type == 'u16_t':
            return struct.unpack('>H', data)[0]
        elif field_type == 'u32_t':
            return struct.unpack('>I', data)[0]
        elif field_type == 'u48_t':
            # Handle 6-byte timestamp
            data = b'\x00\x00' + data  # Pad to 8 bytes
            return struct.unpack('>Q', data)[0]
        elif field_type == 'u64_t':
            return struct.unpack('>Q', data)[0]
        else:
            return data  # Return raw bytes for unknown types

    def parse_message(self, data: bytes) -> Dict[str, Any]:
        """Parse a single ITCH message."""
        if not data:
            return None
            
        message_type = data[0:1].decode('ascii')
        if message_type not in self.message_specs:
            return None
            
        spec = self.message_specs[message_type]
        message = {'message_type': message_type}
        
        for field in spec['fields']:
            start = field['offset']
            end = start + field['length']
            field_data = data[start:end]
            
            value = self.parse_binary_value(field_data, field['type'])
            message[field['name']] = value
            
        return message

    def parse_n_messages(self, binary_file_path: str, n: int) -> List[Dict[str, Any]]:
        """Parse n messages from an ITCH binary file."""
        messages = []
        with open(binary_file_path, 'rb') as f:
            message_count = 0
            while message_count < n:
                # Read message length (2 bytes)
                length_bytes = f.read(2)
                if not length_bytes:
                    break
                    
                message_length = struct.unpack('>H', length_bytes)[0]
                message_data = f.read(message_length)
                
                if len(message_data) < message_length:
                    break  # Incomplete message
                    
                parsed_message = self.parse_message(message_data)
                if parsed_message:
                    messages.append(parsed_message)
                    message_count += 1
                    
        return messages

def format_timestamp(timestamp: int) -> str:
    """Convert ITCH timestamp to human-readable format.
    ITCH timestamps are in nanoseconds since midnight.
    """
    seconds = timestamp // 1_000_000_000
    nanoseconds = timestamp % 1_000_000_000
    microseconds = nanoseconds // 1000
    
    midnight = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
    time = midnight + timedelta(seconds=seconds, microseconds=microseconds)
    return time.strftime('%H:%M:%S.%f')

def format_duration_ns(ns: int) -> str:
    """Format a duration in nanoseconds to a human-readable string."""
    if ns < 1000:
        return f"{ns}ns"
    elif ns < 1_000_000:
        return f"{ns/1000:.3f}µs"
    elif ns < 1_000_000_000:
        return f"{ns/1_000_000:.3f}ms"
    else:
        return f"{ns/1_000_000_000:.3f}s"

def main():
    parser = argparse.ArgumentParser(description='Parse NASDAQ ITCH messages')
    parser.add_argument('num_messages', type=int, help='Number of messages to parse')
    # parser.add_argument('file', help='Binary file containing ITCH messages')
    # parser.add_argument('--xml', default='nasdaq_totalview_itch.xml',
    #                   help='XML specification file (default: nasdaq_totalview_itch.xml)')
    args = parser.parse_args()

    # Initialize parser
    itch_parser = ITCHMessageParser(XML_FILE)
    
    # Measure parsing time with nanosecond precision
    start_time = time.time_ns()
    # messages = itch_parser.parse_n_messages(args.file, args.num_messages)
    messages = itch_parser.parse_n_messages(BIN_FILE, args.num_messages)
    end_time = time.time_ns()
    
    # Calculate duration in nanoseconds
    duration_ns = end_time - start_time
    
    # Print message details
    print("\nMessage Details:")
    for i, msg in enumerate(messages, 1):
        print(f"\nMessage {i}:")
        print(f"Type: {msg['message_type']}")
        if 'timestamp' in msg:
            msg['formatted_time'] = format_timestamp(msg['timestamp'])
            print(f"Time: {msg['formatted_time']}")
        for key, value in msg.items():
            if key not in ['message_type', 'formatted_time']:
                print(f"{key}: {value}")
                
    # Print timing information
    print(f"\n---------------------")
    print(f"\nParsing Performance:")
    print(f"Messages processed: {len(messages)}")
    print(f"Total time: {format_duration_ns(duration_ns)}")
    print(f"Average time per message: {format_duration_ns(duration_ns // len(messages))}")

if __name__ == "__main__":
    main()
