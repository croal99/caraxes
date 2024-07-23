void* memmem(char* haystack, size_t haystack_size, char* needle, size_t needle_size){
    for(;haystack < ((haystack + haystack_size) - needle_size); haystack++){
        for(size_t i = 0; i < needle_size; i++){
            if((char)*(haystack+i) != (char)*(needle+i)){
                continue;
            }
        }
        return haystack;
    }
    return NULL;
}

void find(void) {
    int err;
    
    void* syscall_switch = lookup_name("x64_sys_call");
    printk(KERN_DEBUG "syscall switch function is at 0x%x\n", syscall_switch);

    void* syscall_getdents = lookup_name("__x64_sys_getdents64");
    printk(KERN_DEBUG "syscall getdents is at 0x%x\n", syscall_getdents);

    void* found = memmem(syscall_switch, 1024, (char*)&syscall_getdents, sizeof(void*));
    printk(KERN_DEBUG "found getdents' address in syscall_switch at 0x%x\n", found);

    printk(KERN_DEBUG "syscall switch beginning 0x%x\n", ((const unsigned char *) *syscall_switch));

    void* my_null = NULL;
    // lets null the syscall
    //memcpy(found, &my_null, sizeof(void*));

    return;
}