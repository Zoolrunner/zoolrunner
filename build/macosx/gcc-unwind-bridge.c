/* Original 10.0 Csu predates GCC 3 exception-frame registration. Keep its
 * initialization and then call the compiler's pre-10.2 compatibility hook,
 * in the order used by Apple's later Csu. */
extern void __keymgr_dwarf2_register_sections(void);
extern void __darwin_gcc3_preregister_frame_info(void);

void zr_initialize_gcc_unwind(void) __attribute__((visibility("hidden")));
void zr_initialize_gcc_unwind(void)
{
    __keymgr_dwarf2_register_sections();
    __darwin_gcc3_preregister_frame_info();
}
