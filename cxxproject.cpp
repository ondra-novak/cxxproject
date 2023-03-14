#include <iostream>
#include <fstream>
#include <string_view>
#include <filesystem>
#include <locale>
#include <random>
#include <algorithm>

using std::__cxx11::collate;

#define CMAKE_HEADER "cmake_minimum_required(VERSION 3.1)\n"

#define CMakeLists "CMakeLists.txt"

static std::filesystem::path src("src");
static std::filesystem::path src_tests("src/tests");
static std::filesystem::path conf("conf");
static std::filesystem::path log_path("log");
static std::filesystem::path build("build");
static std::filesystem::path build_debug = build/"debug";
static std::filesystem::path build_release = build/"release";

static void install_makefile() {
    std::ofstream f("Makefile", std::ios::out|std::ios::trunc);
    f <<
"ifdef BUILD_PROFILE\n"
    "\tFORCE_BUILD_PROFILE=__NOT_EXIST__\n"
"else\n"
    "\tFORCE_BUILD_PROFILE=.current_profile.mk\n"
    "\t-include .current_profile.mk\n"
"endif\n"
"\n"
"ifndef BUILD_PROFILE\n"
    "\tBUILD_PROFILE=default_build_profile.conf\n"
"endif\n"
"\n"
"all : all_debug all_release\n"
"all_debug: build/debug/Makefile\n"
    "\t@$(MAKE) --no-print-directory -C build/debug all\n"
"all_release: build/release/Makefile\n"
    "\t@$(MAKE) --no-print-directory -C build/release all\n"
"clean:\n"
    "\t@$(MAKE) --no-print-directory -C build/debug clean\n"
    "\t@$(MAKE) --no-print-directory -C build/release clean\n"
"install:\n"
    "\t@$(MAKE) --no-print-directory -C build/release install\n"
"test:\n"
    "\t@$(MAKE) --no-print-directory -C build/release test\n"
"\n"
"$(FORCE_BUILD_PROFILE):\n"
    "\techo $(FORCE_BUILD_PROFILE)\n"
    "\t$(file >.current_profile.mk,BUILD_PROFILE=$(BUILD_PROFILE))\n"
"\n"
"build/debug/Makefile:  $(BUILD_PROFILE) $(FORCE_BUILD_PROFILE) | build/debug/conf build/debug/log build/debug/data\n"
    "\tmkdir -p build/debug/log\n"
            //this is necesary as the cmake regenerates cache with default build type
    "\trm -f build/debug/CMakeCache.txt\n"
    "\tcmake -G \"Unix Makefiles\" -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug `grep -E -v \"^[[:blank:]]*#\" $(BUILD_PROFILE)`\n"
"\n"
"build/release/Makefile: $(BUILD_PROFILE) $(FORCE_BUILD_PROFILE) | build/release/conf build/release/log build/release/data\n"
    "\tmkdir -p build/release/log\n"
            //this is necesary as the cmake regenerates cache with default build type
    "\trm -f build/release/CMakeCache.txt\n"
    "\tcmake -G \"Unix Makefiles\" -S . -B build/release -DCMAKE_BUILD_TYPE=Release `grep -E -v \"^[[:blank:]]*#\" $(BUILD_PROFILE)`\n"
"\n"
"build/debug/conf: | build/debug conf \n"
     "\tcd build/debug; ln -s ../../conf conf\n"

"build/release/conf: | build/release conf \n"
     "\tcd build/release; ln -s ../../conf conf\n"
"\n"
"build/debug/data: | build/debug data \n"
     "\tcd build/debug; ln -s ../../data data\n"

"build/release/data: | build/release data \n"
     "\tcd build/release; ln -s ../../data data\n"
"\n"
"build/debug/log: | build/debug\n"
     "\tmkdir build/debug/log\n"
"\n"
"build/release/log: | build/release\n"
     "\tmkdir build/release/log\n"
"\n"
"build/debug:\n"
    "\t@mkdir -p build/debug\n"
"\n"
"build/release:\n"
    "\t@mkdir -p build/release\n"
            "\n"
"conf:\n"
    "\t@mkdir -p conf\n"
"\n"
"data:\n"
    "\t@mkdir -p data\n"
"\n"
"distclean:\n"
    "\trm -rfv build\n";
}

static void install_default_build_profile() {
    std::ofstream f("default_build_profile.conf", std::ios::out|std::ios::trunc);
    f << "## type arguments for cmake here, each must start with -D\n"
         "## -DVariable=Value\n"
         "##\n"
         "## Example: use compile 'clang++' \n"
         "# -DCMAKE_CXX_COMPILER=clang++\n"
         "\n";
}

static void install_gitignore() {
    std::ofstream f(".gitignore", std::ios::out|std::ios::trunc);
    f << "/build" << std::endl;
    f << "/.current_profile.mk" << std::endl;

}


static void version_build_files(std::string name) {
    std::string capname;
    std::transform(name.begin(), name.end(), std::back_inserter(capname), toupper);
    std::filesystem::create_directories("version");
    std::ofstream cmake("version/CMakeLists.txt", std::ios::out|std::ios::trunc);
    cmake << "add_custom_target(" << name << "_version\n";
    cmake <<"    ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/version/version.h.in\n";
    cmake <<"          -D DST=${CMAKE_BINARY_DIR}/src/" << name << "_version.h\n";
    cmake <<"          -D ROOT=${CMAKE_SOURCE_DIR}\n";
    cmake <<"          -D GIT_EXECUTABLE=${GIT_EXECUTABLE}\n";
    cmake <<"          -P ${CMAKE_CURRENT_LIST_DIR}/GenerateVersionHeader.cmake\n";
    cmake <<"          )\n";
    cmake.close();

    std::ofstream gen("version/GenerateVersionHeader.cmake", std::ios::out|std::ios::trunc);
    gen << "if(GIT_EXECUTABLE)\n";
    gen << "  get_filename_component(SRC_DIR ${SRC} DIRECTORY)\n";
    gen << "  # Generate a git-describe version string from Git repository tags\n";
    gen << "  execute_process(\n";
    gen << "    COMMAND ${GIT_EXECUTABLE} describe --tags --always\n";
    gen << "    WORKING_DIRECTORY ${SRC_DIR}\n";
    gen << "    OUTPUT_VARIABLE GIT_DESCRIBE_VERSION\n";
    gen << "    RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE\n";
    gen << "    OUTPUT_STRIP_TRAILING_WHITESPACE\n";
    gen << "    )\n";
    gen << "  if(NOT GIT_DESCRIBE_ERROR_CODE)\n";
    gen << "    set(PROJECT_VERSION ${GIT_DESCRIBE_VERSION})\n";
    gen << "  endif()\n";
    gen << "endif()\n";
    gen << "\n";
    gen << "# Final fallback: Just use a bogus version string that is semantically older\n";
    gen << "# than anything else and spit out a warning to the developer.\n";
    gen << "if(NOT DEFINED PROJECT_VERSION)\n";
    gen << "  set(PROJECT_VERSION v0.0.0-unknown)\n";
    gen << "  message(WARNING \"Failed to determine version from Git tags. Using default version \\\"${PROJECT_VERSION}\\\".\")\n";
    gen << "endif()\n";
    gen << "\n";
    gen << "configure_file(${SRC} ${DST} @ONLY)\n";

    gen.close();


    std::ofstream hdr("version/version.h.in", std::ios::out|std::ios::trunc);
    hdr << "#define PROJECT_" << capname << "_VERSION \"@PROJECT_VERSION@\"" << std::endl;
    hdr.close();
}


#define SYSTEM(X) do { int r = system((X)); if (r) throw std::system_error(r, std::system_category(), #X); } while(false)

template<typename Spec>
static void create_project_skeleton(std::string name, Spec spec) {

    std::ofstream f(CMakeLists, std::ios::out| std::ios::trunc);
    if (!f) {
        int e = errno;
        throw std::system_error(e, std::system_category(), std::string("Failed to open ").append(CMakeLists));
    }


    using namespace std::filesystem;

    f << CMAKE_HEADER
         "project (" << name <<  ")\n"
         "set (CMAKE_CXX_STANDARD 20)\n"
         "if (MSVC)\n"
         "\tadd_compile_options(/W4 /EHsc /DNOMINMAX    )\n"
         "\tset(STANDARD_LIBRARIES \"\")\n"
         "else()\n"
         "\tadd_compile_options(-Wall -Wextra -Wpedantic)\n"
         "\tset(STANDARD_LIBRARIES \"pthread\")\n"
         "endif()\n"
         "add_compile_options(-Wall -Wno-noexcept-type)\n"
         "find_package(Git)\n"
         "if(GIT_EXECUTABLE)\n"
         "\texecute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})\n"
         "endif()\n"
         "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)\n"
         "set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)\n"
         "set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)\n"
         "if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)\n"
         "\tset(CMAKE_INSTALL_PREFIX \"/usr/local\" CACHE PATH \"Default path to install\" FORCE)\n"
         "endif()\n"
         "include_directories(BEFORE ${CMAKE_BINARY_DIR}/src)\n";


    create_directories(src);
    create_directories(src/name);

    spec(f);

    f << "add_subdirectory(\"version\")\n";

    f.close();

    SYSTEM("git init");

    install_makefile();
    install_gitignore();
    install_default_build_profile();
    SYSTEM("git add src Makefile " CMakeLists " .gitignore default_build_profile.conf");
    SYSTEM("git commit -m 'Project creation'");
    SYSTEM("git tag 0.0.1");
    std::cout << std::endl;
    std::cout << "------------------" << std::endl;
    std::cout << "DONE!" << std::endl;
    std::cout << "Enter `make` to build the project " << name << "." << std::endl;


}

static void create_main_source(std::string name, bool exec, bool header) {

    std::string capname;
    std::transform(name.begin(), name.end(), std::back_inserter(capname), toupper);

    std::filesystem::path source = src/name/name;
    source.replace_extension("cpp");
    if (!std::filesystem::exists(source)) {
        std::ofstream x(source, std::ios::out);
        x << "#include \"" << name << ".h\"\n\n";
        if (header) {
            x << "#include <" << name << "_version.h>\n";
        }
        if (exec) {
            x << "#include <iostream>\n#include <cstdlib>\n\n";
            x << "int main(int argc, char **argv) {\n    std::cout << \"Version: \" << PROJECT_" << capname << "_VERSION << std::endl;\n    return 0;\n}\n";
        } else {
            x << "namespace " << name << "{\n\n}\n";
        }
    }
}

static std::string generate_guard(std::string name) {

    std::random_device rnd_src;
    std::default_random_engine rnd(rnd_src());
    std::uniform_int_distribution<> rdist(0,2*20+10);

    std::string out = "_"+name+"_src_"+name+"_H_";
    for (int i = 0; i < 32; i++) {
        int v = rdist(rnd);
        char c;
        if (v < 10) c = '0'+v;
        else if (v < 30) c = 'A'+(v-10);
        else c = 'a' + (v-30);
        out.push_back(c);
    }
    out.push_back('_');
    return out;
}

static void create_main_header(std::string name, bool exec) {
    std::filesystem::path source = src/name/name;
    source.replace_extension("h");
    if (!std::filesystem::exists(source)) {
        std::ofstream x(source, std::ios::out);
        std::string guard = generate_guard(name);
        x << "#pragma once\n";
        x << "#ifndef " << guard << "\n";
        x << "#define " << guard << "\n\n";
        if (!exec) {
            x << "namespace " << name << "{\n\n}\n\n";
        }
        x << "#endif /* " << guard << " */\n";
    }}

static void create_test_source(std::string name, std::string test_dir) {
    std::filesystem::path source = src/test_dir/"compile_test.cpp";
    if (!std::filesystem::exists(source)) {
        std::ofstream x(source, std::ios::out);
        if (!name.empty()) {
            x << "#include <" << name << "/" << name << ".h>\n\n";
        }
        x << "#include <iostream>\n#include <cstdlib>\n\n";
        if (!name.empty()) {
            x <<"using namespace " << name << ";\n\n";
        }
        x << "int main(int argc, char **argv) {\n    return 0;\n}\n";
    }
}

static void create_exec_cmake(std::string name, bool version) {
    std::string sublists = src/name/CMakeLists;
    std::ofstream f(sublists, std::ios::out| std::ios::trunc);
    if (!f) {
        int e = errno;
        throw std::system_error(e, std::system_category(), "Failed to open "+sublists);
    }
    f <<  CMAKE_HEADER "\n";
    f << "add_executable(" << name << "\n\t" << name << ".cpp\n)\n\n";
    f << "target_link_libraries(" << name << "\n\t${STANDARD_LIBRARIES}\n)\n";
    if (version)
        f << "add_dependencies(" << name << " " << name << "_version)\n\n";

}

static void create_lib_cmake(std::string name, bool version) {
    std::string sublists = src/name/CMakeLists;
    std::ofstream f(sublists, std::ios::out| std::ios::trunc);
    if (!f) {
        int e = errno;
        throw std::system_error(e, std::system_category(), "Failed to open "+sublists);
    }
    f << CMAKE_HEADER "\n";
    f << "add_library(" << name << "\n\t" << name << ".cpp\n)\n";
    if (version)
        f << "add_dependencies(" << name << " " << name << "_version)\n\n";

}

static void create_test_cmake(std::string name, std::string test_dir) {
    std::string sublists = src/test_dir/CMakeLists;
    std::ofstream f(sublists, std::ios::out| std::ios::trunc);
    if (!f) {
        int e = errno;
        throw std::system_error(e, std::system_category(), "Failed to open "+sublists);
    }
    f << CMAKE_HEADER "\n";
    f << "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/)\n"
         "#file(GLOB testFiles \"*.cpp\")\n"
         "set(testFiles \n"
         "\tcompile_test.cpp\n"
         ")\n\n"

         "link_libraries(\n\t" << name << "\n\t${STANDARD_LIBRARIES}\n)\n\n"

        "foreach (testFile ${testFiles})\n"
            "\tstring(REGEX MATCH \"([^\\/]+$)\" filename ${testFile})\n"
            "\tstring(REGEX MATCH \"[^.]*\" executable_name " << test_dir << "_${filename})\n"
            "\tadd_executable(${executable_name} ${testFile})\n"
            "\ttarget_link_libraries(${executable_name} ${STANDARD_LIBRARIES} )\n"
            "\tadd_test(NAME \"" << test_dir << "/${filename}\" COMMAND ${executable_name})\n"
        "endforeach ()\n";


}

static void create_library_dot_cmake(std::string name) {
    std::ofstream f("library.cmake", std::ios::out| std::ios::trunc);
    if (!f) {
        int e = errno;
        throw std::system_error(e, std::system_category(), "Failed to open library.cmake");
    }
    f << "include_directories(AFTER ${CMAKE_CURRENT_LIST_DIR}/src)\n";
    f << "add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/src/" << name << " EXCLUDE_FROM_ALL)\n";
}

static int create_exec(std::string name) {

    if (name == "tests") throw std::runtime_error("Name 'tests' cannot be used");


    create_project_skeleton(name, [=](std::ostream &out){

        out << "include_directories(AFTER src)\n";
        out << "add_subdirectory(\"src/" << name << "\")\n";

        version_build_files(name);
        create_main_source(name, true, true);
        create_main_header(name, true);
        create_exec_cmake(name, true);


    });
    return 0;
}

static int create_lib(std::string name) {

    if (name == "tests") throw std::runtime_error("Name 'tests' cannot be used");

    create_project_skeleton(name, [=](std::ostream &out){

        out << "include(library.cmake)\n";
        out << "enable_testing()\n";
        out << "add_subdirectory(\"src/tests\")\n";

        version_build_files(name);
        create_directories(src_tests);
        create_main_source(name, false, true);
        create_main_header(name, false);
        create_test_source(name, "tests");
        create_test_cmake(name, "tests");
        create_library_dot_cmake(name);
        create_lib_cmake(name,true);


    });
    SYSTEM("git add library.cmake");


    return 0;

}

template<typename Fn>
static void insert_to_cmake(Fn &&fn) {
    std::string fname = CMakeLists;
    std::string fname_new = fname+".part";
    std::string ln;

    std::ifstream in(fname);
    if (!in) throw std::runtime_error("Can't open "+fname);
    std::ofstream out(fname_new, std::ios::out|std::ios::trunc);

    bool inserted = false;

    while (!in.eof()) {
        std::getline(in, ln);
        if (ln.find("add_subdirectory(") != ln.npos && !inserted) {
            inserted = true;
            fn(out);
        }
        out << ln << std::endl;
    }
    if (!inserted) {
        fn(out);
    }
    in.close();
    out.close();

    std::filesystem::rename(fname_new, fname);
}


static int add_empty_lib(std::string name) {
    using namespace std::filesystem;
    auto path = src/name;
    if (exists(path)) {
        throw std::runtime_error("already exists");
    }
    std::string p = path;

    insert_to_cmake([&](std::ostream &out){
        out << "add_subdirectory(\""<< p << "\")\n";
    });
    create_directories(path);
    create_main_source(name, false, false);
    create_main_header(name, false);
    create_lib_cmake(name,false);

    SYSTEM(("git add "+p).c_str());
    return 0;
}

static int add_empty_exec(std::string name) {
    using namespace std::filesystem;
    auto path = src/name;
    if (exists(path)) {
        throw std::runtime_error("already exists");
    }
    std::string p = path;

    insert_to_cmake([&](std::ostream &out){
        out << "add_subdirectory(\""<< p << "\")\n";
    });
    create_directories(path);
    create_main_source(name, true, false);
    create_main_header(name, true);
    create_exec_cmake(name,false);

    SYSTEM(("git add "+p).c_str());
    return 0;
}

static int add_tests(std::string name) {
    using namespace std::filesystem;
    auto path = src/name;
    if (exists(path)) {
        throw std::runtime_error("already exists");
    }
    std::string p = path;

    insert_to_cmake([&](std::ostream &out){
        out << "add_subdirectory(\""<< p << "\")\n";
    });
    create_directories(path);
    create_test_source("", name);
    create_test_cmake("",name);

    SYSTEM(("git add "+p).c_str());
    return 0;
}

static int add_git_lib(std::string name, std::string url, std::string branch) {
    using namespace std::filesystem;
    auto path = src/name;
    if (exists(path)) {
        throw std::runtime_error("already exists");
    }

    auto cmd = std::string("git submodule add ");
    if (!branch.empty()) cmd.append("-b ").append(branch).append(" ");
    cmd.append(url).append(" ").append(path);
    if (system(cmd.c_str()) != 0) {
        throw std::runtime_error("Git command failed, stop");
    }

    auto libcmake = src/name/"library.cmake";
    if (exists(libcmake)) {
        insert_to_cmake([&](std::ostream &out) {
            out << "include(" << libcmake.string() << ")\n";
        });
    }
    return 0;
}



int main(int argc, char **argv) {

    try {
    if (argc > 1) {

        std::string_view arg1 = argv[1];

        if (arg1 == "-h" || arg1 == "--help") {
            std::cout << "Usage: cxxproject <command> <args...>\n"
                    "\n"
                    "create executable <name>        create C++ executable\n"
                    "create library <name>           create C++ library\n"
                    "add library <name>              add empty library\n"
                    "add library <name> <gitpath>    add library from git\n"
                    "add library <name> <gitpath> <branch>\n"
                    "                                add library from git - branch\n"
                    "                (automatically includes library.cmake if exists)\n"
                    "add executable <name>           add new executable\n"
                    "add tests <name>                add new test directory\n";
            return 0;
        }
        if (arg1 == "create") {
            if (argc > 3) {
                std::string_view arg2 = argv[2];
                std::string_view arg3 = argv[3];
                if (arg2 == "executable") {
                    return create_exec(std::string(arg3));
                } else if (arg2 == "library") {
                    return create_lib(std::string(arg3));
                }
            }
        } else if (arg1 == "add") {
            if (argc > 3) {
                std::string_view arg2 = argv[2];
                std::string_view arg3 = argv[3];
                if (arg2 == "library") {
                    if (argc > 4) {
                        std::string_view arg4 = argv[4];
                        std::string_view branch;
                        if (argc > 5) {
                            branch = argv[5];
                        }
                        return add_git_lib(std::string(arg3), std::string(arg4), std::string(branch));
                    } else if (arg3.find('/') != arg3.npos){
                        throw std::runtime_error(std::string("Invalid library name: ").append(arg3));;
                    } else {
                        return add_empty_lib(std::string(arg3));
                    }
                }
                if (arg2 == "executable") {
                    return add_empty_exec(std::string(arg3));
                }
                if (arg2 == "tests") {
                    return add_tests(std::string(arg3));
                }
            }
        }

    }
    std::cerr << "Use -h for help" << std::endl;
    return 1;
    } catch (const std::exception &e) {
        std::cerr << "Fatal:" << e.what() << std::endl;
        return 128;
    }
}
