#include "dwarf_utils.h"

unsigned long long get_address_from_function(Dwarf_Debug dbg, Dwarf_Die current_die) {
    Dwarf_Attribute* attributes;
    Dwarf_Addr lowpc;
    Dwarf_Signed number_of_attr;
    Dwarf_Error error;
    unsigned long long address;
    char string_address[16]; 

    if (dwarf_attrlist(current_die, &attributes, &number_of_attr, &error) != DW_DLV_OK)
        printf("\nError in dwarf_attlist");

    for (int i = 0; i < number_of_attr; ++i) {
        Dwarf_Half attribute_code;
        if (dwarf_whatattr(attributes[i], &attribute_code, &error) != DW_DLV_OK)
            printf("\nError in dwarf_whatattr\n");
        if (attribute_code == DW_AT_low_pc) {
            dwarf_formaddr(attributes[i], &lowpc, 0);
            sprintf(string_address, "%llx", lowpc);
            sscanf(string_address, "%llx", &address);
            return address;
        }
    }
    return 0;
}

bool is_function(Dwarf_Debug dbg, Dwarf_Die current_die, char *function_name) {
    char* die_name = 0;
    const char* tag_name = 0;
    Dwarf_Error error;
    Dwarf_Half tag;
    int rc = dwarf_diename(current_die, &die_name, &error);

    if (rc == DW_DLV_ERROR)
        printf("Error in dwarf_diename\n");
    else if (rc == DW_DLV_NO_ENTRY)
        return false;
    if (dwarf_tag(current_die, &tag, &error) != DW_DLV_OK)
        printf("\nError in dwarf_tag");

    if (tag != DW_TAG_subprogram)
        return false;
    if (dwarf_get_TAG_name(tag, &tag_name) != DW_DLV_OK)
        printf("Error in dwarf_get_TAG_name\n");

    if(strcmp(die_name, function_name) == 0) {
        return true;
    }
    return false;
}

unsigned long long process_cu(char* program_name, char* function_name) {
    Dwarf_Debug dbg = 0;
	int fd = -1;
	int res = DW_DLV_ERROR;
	Dwarf_Error error;
	Dwarf_Handler error_handler = 0;
	Dwarf_Ptr error_arg = 0;
    fd = open(program_name, O_RDONLY);
    res = dwarf_init(fd,DW_DLC_READ, error_handler, error_arg, &dbg, &error);
    if(res != DW_DLV_OK) {
		printf("\nGiving up, cannot do DWARF processing.");
		exit(1);
	}

	Dwarf_Unsigned cu_header_length, abbrev_offset, next_cu_header;
    Dwarf_Half version_stamp, address_size;
    Dwarf_Error err;
    Dwarf_Die no_die = 0, cu_die, child_die;

    if (dwarf_next_cu_header(
                dbg,
                &cu_header_length,
                &version_stamp,
                &abbrev_offset,
                &address_size,
                &next_cu_header,
                &err) == DW_DLV_ERROR)
        printf("\nError reading DWARF cu header.");

    if (dwarf_siblingof(dbg, no_die, &cu_die, &err) == DW_DLV_ERROR)
        printf("\nError getting sibling of CU");

    if (dwarf_child(cu_die, &child_die, &err) == DW_DLV_ERROR)
        printf("\nError getting child of CU DIE");

    while (1) {
        int rc;
        bool found_function = is_function(dbg, child_die, function_name);
        if(found_function) {
            unsigned long long address = get_address_from_function(dbg, child_die);
            printf("\nFound function %s at addr %llx", function_name
            , address);
            return address;
        }

        rc = dwarf_siblingof(dbg, child_die, &child_die, &err);

        if (rc == DW_DLV_ERROR)
            printf("\nError getting sibling of DIE");
        else if (rc == DW_DLV_NO_ENTRY)
            break;
    }
    res = dwarf_finish(dbg,&error);
	if(res != DW_DLV_OK) {
		printf("\ndwarf_finish failed!");
	}
	close(fd);
}