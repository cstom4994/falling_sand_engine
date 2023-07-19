
#ifndef ME_DBGTOOLS_H
#define ME_DBGTOOLS_H

namespace ME {

typedef struct {
    const char* function;  ///< name of function containing address of function.
    const char* file;      ///< file where symbol is defined, might not work on all platforms.
    unsigned int line;     ///< line in file where symbol is defined, might not work on all platforms.
    unsigned int offset;   ///< offset from start of function where call was made.
} callstack_symbol_t;

/**
 * Generate a callstack from the current location in the code.
 * @param skip_frames number of frames to skip in output to addresses.
 * @param addresses is a pointer to a buffer where to store addresses in callstack.
 * @param num_addresses size of addresses.
 * @return number of addresses in callstack.
 */
int callstack(int skip_frames, void** addresses, int num_addresses);

/**
 * Translate addresses from, for example, callstack to symbol-names.
 * @param addresses list of pointers to translate.
 * @param out_syms list of callstack_symbol_t to fill with translated data, need to fit as many strings as there are ptrs in addresses.
 * @param num_addresses number of addresses in addresses
 * @param memory memory used to allocate strings stored in out_syms.
 * @param mem_size size of addresses.
 * @return number of addresses translated.
 *
 * @note On windows this will load dbghelp.dll dynamically from the following paths:
 *       1) same path as the current module (.exe)
 *       2) current working directory.
 *       3) the usual search-paths ( PATH etc ).
 *
 *       Some thing to be wary of is that if you are using symbol-server functionality symsrv.dll MUST reside together with
 *       the dbghelp.dll that is loaded as dbghelp.dll will only load that from the same path as where it self lives.
 *
 * @note On windows .pdb search paths will be set in the same way as dbghelp-defaults + the current module (.exe) dir, i.e.:
 *       1) same path as the current module (.exe)
 *       2) current working directory.
 *       3) The _NT_SYMBOL_PATH environment variable.
 *       4) The _NT_ALTERNATE_SYMBOL_PATH environment variable.
 *
 * @note On platforms that support it debug-output can be enabled by defining the environment variable DBGTOOLS_SYMBOL_DEBUG_OUTPUT.
 */
int callstack_symbols(void** addresses, callstack_symbol_t* out_syms, int num_addresses, char* memory, int mem_size);

void print_callstack();

}  // namespace ME

#endif
