#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

// Constants
#define MAX_FIELDS 20
#define MAX_MESSAGE_TYPES 32
#define MAX_FIELD_NAME 64
#define MAX_TYPE_NAME 32

// Define here the TotalView ITCH 5.0 messages bin file to process with this script. 
// These files are quite large, eg. over 10GB, and are thus omitted in this repo. 
// Find one here: https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/
#define BINARY_FILE_PATH "../01302019.NASDAQ_ITCH50"
// The reference TotalView ITCH 5.0 protocol breakdown, used to parse the bin file
#define XML_FILE_PATH "../nasdaq_totalview_itch.xml"

// Endianness conversion functions for macOS
#define be16toh(x) manual_be16toh(x)
#define be32toh(x) manual_be32toh(x)
#define be64toh(x) manual_be64toh(x)

// Manual big-endian to host conversion functions
uint16_t manual_be16toh(uint16_t big_endian) {
    unsigned char *bytes = (unsigned char *)&big_endian;
    return ((uint16_t)bytes[0] << 8) |
            (uint16_t)bytes[1];
}

uint32_t manual_be32toh(uint32_t big_endian) {
    unsigned char *bytes = (unsigned char *)&big_endian;
    return ((uint32_t)bytes[0] << 24) |
           ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) |
            (uint32_t)bytes[3];
}

uint64_t manual_be64toh(uint64_t big_endian) {
    unsigned char *bytes = (unsigned char *)&big_endian;
    return ((uint64_t)bytes[0] << 56) |
           ((uint64_t)bytes[1] << 48) |
           ((uint64_t)bytes[2] << 40) |
           ((uint64_t)bytes[3] << 32) |
           ((uint64_t)bytes[4] << 24) |
           ((uint64_t)bytes[5] << 16) |
           ((uint64_t)bytes[6] << 8) |
            (uint64_t)bytes[7];
}

// Field type enums
typedef enum {
    TYPE_CHAR,
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U48,
    TYPE_U64,
    TYPE_PRICE4,
    TYPE_PRICE8,
    TYPE_CHAR_ARRAY
} FieldType;

// Struct to hold field information
typedef struct {
    char name[MAX_FIELD_NAME];
    int offset;
    int length;
    FieldType type;
} Field;

// Struct to hold message type information
typedef struct {
    char id;
    int length;
    int num_fields;
    Field fields[MAX_FIELDS];
} MessageSpec;

// Global variables to store message specifications
MessageSpec message_specs[MAX_MESSAGE_TYPES];
int num_message_specs = 0;

// Function to convert field type string to enum
FieldType get_field_type(const char* type_str) {
    if (strncmp(type_str, "char_t", 6) == 0) return TYPE_CHAR;
    if (strncmp(type_str, "u8_t", 4) == 0) return TYPE_U8;
    if (strncmp(type_str, "u16_t", 5) == 0) return TYPE_U16;
    if (strncmp(type_str, "u32_t", 5) == 0) return TYPE_U32;
    if (strncmp(type_str, "u48_t", 5) == 0) return TYPE_U48;
    if (strncmp(type_str, "u64_t", 5) == 0) return TYPE_U64;
    if (strncmp(type_str, "price_4_t", 9) == 0) return TYPE_PRICE4;
    if (strncmp(type_str, "price_8_t", 9) == 0) return TYPE_PRICE8;
    if (strncmp(type_str, "char_", 5) == 0) return TYPE_CHAR_ARRAY;
    return TYPE_CHAR; // Default
}

// Function to parse XML specification
void parse_xml_spec(const char* xml_path) {
    xmlDoc *doc = xmlReadFile(xml_path, NULL, 0);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse XML file\n");
        return;
    }

    xmlNode *root = xmlDocGetRootElement(doc);
    xmlNode *structs = root->children;

    // Find Structs node
    while (structs != NULL && strcmp((char*)structs->name, "Structs") != 0) {
        structs = structs->next;
    }

    if (structs == NULL) {
        fprintf(stderr, "No Structs section found in XML\n");
        xmlFreeDoc(doc);
        return;
    }

    // Parse each Struct
    xmlNode *struct_node = structs->children;
    while (struct_node != NULL) {
        if (struct_node->type == XML_ELEMENT_NODE && strcmp((char*)struct_node->name, "Struct") == 0) {
            xmlChar *id = xmlGetProp(struct_node, (xmlChar*)"id");
            xmlChar *len = xmlGetProp(struct_node, (xmlChar*)"len");

            if (id != NULL && len != NULL) {
                MessageSpec *spec = &message_specs[num_message_specs];
                spec->id = ((char*)id)[0];
                spec->length = atoi((char*)len);
                spec->num_fields = 0;

                // Parse fields
                xmlNode *field = struct_node->children;
                while (field != NULL) {
                    if (field->type == XML_ELEMENT_NODE && strcmp((char*)field->name, "Field") == 0) {
                        xmlChar *name = xmlGetProp(field, (xmlChar*)"name");
                        xmlChar *offset = xmlGetProp(field, (xmlChar*)"offset");
                        xmlChar *length = xmlGetProp(field, (xmlChar*)"len");
                        xmlChar *type = xmlGetProp(field, (xmlChar*)"type");

                        if (name && offset && length && type) {
                            Field *f = &spec->fields[spec->num_fields];
                            strncpy(f->name, (char*)name, MAX_FIELD_NAME - 1);
                            f->offset = atoi((char*)offset);
                            f->length = atoi((char*)length);
                            f->type = get_field_type((char*)type);
                            spec->num_fields++;
                        }

                        xmlFree(name);
                        xmlFree(offset);
                        xmlFree(length);
                        xmlFree(type);
                    }
                    field = field->next;
                }
                num_message_specs++;
            }

            xmlFree(id);
            xmlFree(len);
        }
        struct_node = struct_node->next;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

// Function to parse binary value based on field type
void parse_binary_value(const unsigned char* data, Field* field, char* output, int output_size) {
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    char temp[64];

    switch (field->type) {
        case TYPE_CHAR:
            snprintf(output, output_size, "%c", data[0]);
            break;
        case TYPE_U8:
            snprintf(output, output_size, "%u", data[0]);
            break;
        case TYPE_U16:
            memcpy(&u16, data, sizeof(u16));
            u16 = be16toh(u16);  // Convert from big-endian
            snprintf(output, output_size, "%u", u16);
            break;
        case TYPE_U32:
            memcpy(&u32, data, sizeof(u32));
            u32 = be32toh(u32);  // Convert from big-endian
            snprintf(output, output_size, "%u", u32);
            break;
        case TYPE_U48:
            memcpy(temp, "\x00\x00", 2);
            memcpy(temp + 2, data, 6);
            memcpy(&u64, temp, sizeof(u64));
            u64 = be64toh(u64);  // Convert from big-endian
            snprintf(output, output_size, "%lu", u64);
            break;
        case TYPE_U64:
            memcpy(&u64, data, sizeof(u64));
            u64 = be64toh(u64);  // Convert from big-endian
            snprintf(output, output_size, "%lu", u64);
            break;
        case TYPE_PRICE4:
            memcpy(&u32, data, sizeof(u32));
            u32 = be32toh(u32);  // Convert from big-endian
            snprintf(output, output_size, "%.4f", u32 / 10000.0);
            break;
        case TYPE_PRICE8:
            memcpy(&u64, data, sizeof(u64));
            u64 = be64toh(u64);  // Convert from big-endian
            snprintf(output, output_size, "%.8f", u64 / 100000000.0);
            break;
        case TYPE_CHAR_ARRAY:
            memcpy(temp, data, field->length);
            temp[field->length] = '\0';
            snprintf(output, output_size, "%s", temp);
            break;
        default:
            snprintf(output, output_size, "<unknown>");
    }
}

// Function to find message specification by ID
MessageSpec* find_message_spec(char id) {
    for (int i = 0; i < num_message_specs; i++) {
        if (message_specs[i].id == id) {
            return &message_specs[i];
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_messages>\n", argv[0]);
        return 1;
    }

    int num_messages = atoi(argv[1]);

    // Parse XML specification
    parse_xml_spec(XML_FILE_PATH);

    // Open binary file
    FILE *file = fopen(BINARY_FILE_PATH, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open binary file: %s\n", BINARY_FILE_PATH);
        return 1;
    }

    // Variables for timing
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Process messages
    int messages_processed = 0;
    char value_buffer[256];
    
    while (messages_processed < num_messages) {
        // Read message length
        uint16_t message_length;
        if (fread(&message_length, sizeof(uint16_t), 1, file) != 1) break;
        message_length = be16toh(message_length);  // Convert from big-endian

        // Read message data
        unsigned char *message_data = malloc(message_length);
        if (fread(message_data, 1, message_length, file) != message_length) {
            free(message_data);
            break;
        }

        // Get message type and find specification
        char message_type = message_data[0];
        MessageSpec *spec = find_message_spec(message_type);

        if (spec != NULL) {
            printf("\nMessage %d:\n", messages_processed + 1);
            printf("Type: %c\n", message_type);

            // Parse each field
            for (int i = 0; i < spec->num_fields; i++) {
                Field *field = &spec->fields[i];
                parse_binary_value(message_data + field->offset, field, value_buffer, sizeof(value_buffer));
                printf("%s: %s\n", field->name, value_buffer);
            }

            messages_processed++;
        }

        free(message_data);
    }

    // Calculate and print timing information
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t duration_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                          (end.tv_nsec - start.tv_nsec);

    printf("\nParsing Performance:\n");
    printf("Messages processed: %d\n", messages_processed);
    printf("Total time: %.3f ms\n", duration_ns / 1000000.0);
    printf("Average time per message: %.3f µs\n", (duration_ns / messages_processed) / 1000.0);

    fclose(file);
    return 0;
}
