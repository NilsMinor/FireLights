if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(ENABLE_SANITIZERS)
    set(SANITIZER  "-fsanitize=address -fsanitize=undefined -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=enum -fsanitize=bool -fsanitize=vptr -fsanitize=pointer-overflow")
  else(ENABLE_SANITIZERS)
    set(SANITIZER  "")
  endif(ENABLE_SANITIZERS)
  
  set(CMAKE_C_FLAGS           "-include ${PROJECT_SOURCE_DIR}/cherrysim/SystemTest.h -m32 -Wno-unknown-pragmas -fno-builtin -fno-strict-aliasing -fomit-frame-pointer -std=gnu99" CACHE INTERNAL "c compiler flags")
  set(CMAKE_CXX_FLAGS         "-include ${PROJECT_SOURCE_DIR}/cherrysim/SystemTest.h -m32 -Wno-unknown-pragmas -fprofile-arcs -ftest-coverage -fno-builtin -fno-strict-aliasing -fomit-frame-pointer -fdata-sections -ffunction-sections -fsingle-precision-constant -std=c++17 -pthread ${SANITIZER} -fno-omit-frame-pointer " CACHE INTERNAL "cxx compiler flags")
  set(CMAKE_EXE_LINKER_FLAGS  "-rdynamic -fprofile-arcs -ftest-coverage ${SANITIZER} -fno-omit-frame-pointer"  CACHE INTERNAL "exe link flags")

  set(CMAKE_C_FLAGS_DEBUG     "-Og -g3 -ggdb3"  CACHE INTERNAL "c debug compiler flags")
  set(CMAKE_CXX_FLAGS_DEBUG   "-Og -g3 -ggdb3"  CACHE INTERNAL "cxx debug compiler flags")
  set(CMAKE_ASM_FLAGS_DEBUG   "-g -ggdb3"       CACHE INTERNAL "asm debug compiler flags")
   
  set(CMAKE_C_FLAGS_RELEASE   "-O3 -g3 -ggdb3"  CACHE INTERNAL "c release compiler flags")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g3 -ggdb3"  CACHE INTERNAL "cxx release compiler flags")
  set(CMAKE_ASM_FLAGS_RELEASE "-g3 -ggdb3"      CACHE INTERNAL "asm release compiler flags")
    
  set(CMAKE_C_FLAGS_MINSIZEREL   "-Os -g3 -ggdb3"   CACHE INTERNAL "c mininum size compiler flags")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -g3 -ggdb3"   CACHE INTERNAL "cxx mininum size compiler flags")
  set(CMAKE_ASM_FLAGS_MINSIZEREL "-g3 -ggdb3"       CACHE INTERNAL "asm mininum size compiler flags")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wall")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wextra")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Werror")
  target_compile_options_multi_lang("${SIMULATOR_TARGETS}" CXX "-Wno-terminate") # C does not have such a warning.
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-unused-but-set-variable")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-vla")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-unused-parameter")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-type-limits")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-sign-compare")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-missing-field-initializers") # Overly paranoid warning that hinders value initialization (a = {})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "/FI ${CMAKE_CURRENT_LIST_DIR}/../cherrysim/SystemTest.h")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "/std:c++latest")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "/MP")
  target_link_options_multi("${SIMULATOR_TARGETS}" "/DYNAMICBASE:NO") # Disables ASLR. Helps us to debug as we can create a watch on addresses of previous runs.
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wall")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wextra")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Werror")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Warray-bounds-pointer-arithmetic")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wbad-function-cast")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wbitfield-enum-conversion")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-unknown-pragmas")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-unused-parameter")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-unused-const-variable")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-constant-logical-operand")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-Wno-missing-field-initializers") # Overly paranoid warning that hinders value initialization (a = {})
  target_compile_options_multi("${SIMULATOR_TARGETS}" "--include=${PROJECT_SOURCE_DIR}/cherrysim/SystemTest.h")
  target_compile_options_multi("${SIMULATOR_TARGETS}" "-m32")
  set(CMAKE_C_FLAGS           "-fno-builtin -fno-strict-aliasing -fomit-frame-pointer -std=gnu99" CACHE INTERNAL "c compiler flags")
  set(CMAKE_CXX_FLAGS         "-fprofile-arcs -ftest-coverage -fno-builtin -fno-strict-aliasing -fomit-frame-pointer -fdata-sections -ffunction-sections -std=c++17 -pthread -fno-omit-frame-pointer " CACHE INTERNAL "cxx compiler flags")
  set(CMAKE_EXE_LINKER_FLAGS  "-rdynamic -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer"  CACHE INTERNAL "exe link flags")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")