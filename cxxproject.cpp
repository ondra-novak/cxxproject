#include <iostream>
#include <fstream>
#include <string_view>
#include <filesystem>

#define CMAKE_HEADER "cmake_minimum_required(VERSION 3.1)\n"

#define CMakeLists "CMakeLists.txt"

static std::filesystem::path src("src");
static std::filesystem::path src_tests("src/tests");
static std::filesystem::path conf("conf");
static std::filesystem::path log("log");
static std::filesystem::path build("build");
static std::filesystem::path build_debug = build/"debug";
static std::filesystem::path build_release = build/"release";

static void install_makefile() {
    std::ofstream f("Makefile", std::ios::out|std::ios::trunc);
    f <<
"ifdef BUILD_PROFILE\n"
    "\tFORCE_BUILD_PROFILE=__NOT_EXIST__\n"
"else\n"
    "\tFORCE_BUILD_PROFILE=current_profile.mk\n"
    "\t-include current_profile.mk\n"
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
"\n"
"$(FORCE_BUILD_PROFILE):\n"
    "\techo $(FORCE_BUILD_PROFILE)\n"
    "\t$(file >current_profile.mk,BUILD_PROFILE=$(BUILD_PROFILE))\n"
"\n"
"build/debug/Makefile:  $(BUILD_PROFILE) $(FORCE_BUILD_PROFILE) | build/debug/conf build/debug/log build/debug/data\n"
    "\tmkdir -p build/debug/log\n"
    "\tcmake -G \"Unix Makefiles\" -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug `grep -E -v \"^[[:blank:]]*#\" $(BUILD_PROFILE)`\n"
"\n"
"build/release/Makefile: $(BUILD_PROFILE) $(FORCE_BUILD_PROFILE) | build/release/conf build/release/log build/release/data\n"
    "\tmkdir -p build/release/log\n"
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
    f << "/current_profile.mk" << std::endl;

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
         "add_compile_options(-Wall -Wno-noexcept-type)\n"
         "exec_program(\"git submodule update --init\")\n"
         "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)\n"
         "set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)\n"
         "set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)\n"
         "if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)\n"
         "\tset(CMAKE_INSTALL_PREFIX \"/usr/local\" CACHE PATH \"Default path to install\" FORCE)\n"
         "endif()\n";


    create_directories(src);
    create_directories(src/name);

    spec(f);

    f.close();

    SYSTEM("git init");

    install_makefile();
    install_gitignore();
    install_default_build_profile();
    SYSTEM("git add src Makefile " CMakeLists " .gitignore default_build_profile.conf");
    std::cout << std::endl;
    std::cout << "------------------" << std::endl;
    std::cout << "DONE!" << std::endl;
    std::cout << "Enter `make` to build the project " << name << "." << std::endl;


}

static void create_main_source(std::string name, bool exec) {
    std::filesystem::path source = src/name/name;
    source.replace_extension("cpp");
    if (!std::filesystem::exists(source)) {
        std::ofstream x(source, std::ios::out);
        x << "#include \"" << name << ".h\"\n\n";
        if (exec) {
            x << "int main(int argc, char **argv) {\n    return 0;\n}\n";
        } else {
            x << "namespace " << name << "{\n\n}\n";
        }
    }
}

static void create_main_header(std::string name, bool exec) {
    std::filesystem::path source = src/name/name;
    source.replace_extension("h");
    if (!std::filesystem::exists(source)) {
        std::ofstream x(source, std::ios::out);
        if (!exec) {
            x << "namespace " << name << "{\n\n}\n";
        }
    }}

static void create_test_source(std::string name) {
    std::filesystem::path source = src_tests/"compile_test.cpp";
    if (!std::filesystem::exists(source)) {
        std::ofstream x(source, std::ios::out);
        x << "#include <" << name << "/" << name << ".h>\n\nusing namespace "
                << name
                << ";\n\nint main(int argc, char **argv) {\n    return 0;\n}\n";
    }
}

static void create_exec_cmake(std::string name) {
    std::string sublists = src/name/CMakeLists;
    std::ofstream f(sublists, std::ios::out| std::ios::trunc);
    if (!f) {
        int e = errno;
        throw std::system_error(e, std::system_category(), "Failed to open "+sublists);
    }
    f <<  CMAKE_HEADER "\n";
    f << "add_executable(" << name << "\n\t" << name << ".cpp\n)\n\n";
    f << "target_link_libraries(" << name << "\n\tpthread\n)\n\n";

}

static void create_lib_cmake(std::string name) {
    std::string sublists = src/name/CMakeLists;
    std::ofstream f(sublists, std::ios::out| std::ios::trunc);
    if (!f) {
        int e = errno;
        throw std::system_error(e, std::system_category(), "Failed to open "+sublists);
    }
    f << CMAKE_HEADER "\n";
    f << "add_library(" << name << "\n\t" << name << ".cpp\n)\n\n";

}

static void create_test_cmake(std::string name) {
    std::string sublists = src_tests/CMakeLists;
    std::ofstream f(sublists, std::ios::out| std::ios::trunc);
    if (!f) {
        int e = errno;
        throw std::system_error(e, std::system_category(), "Failed to open "+sublists);
    }
    f << CMAKE_HEADER "\n";
    f << "add_executable(compile_test compile_test.cpp)\n";
    f << "target_link_libraries(compile_test\n\t" << name << "\n\tpthread\n)\n\n";

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


    create_project_skeleton(name, [=](std::ostream &out){

        out << "include_directories(AFTER src)\n";
        out << "add_subdirectory(\"src/" << name << "\")\n";

        create_main_source(name, true);
        create_main_header(name, true);
        create_exec_cmake(name);


    });
    return 0;
}

static int create_lib(std::string name) {

    if (name == "tests") throw std::runtime_error("Name 'tests' cannot be used");

    create_project_skeleton(name, [=](std::ostream &out){

        out << "include(library.cmake)\n";
        out << "add_subdirectory(\"src/tests\")\n";

        create_directories(src_tests);
        create_main_source(name, false);
        create_main_header(name, false);
        create_test_source(name);
        create_test_cmake(name);
        create_library_dot_cmake(name);
        create_lib_cmake(name);


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
    create_main_source(name, false);
    create_main_header(name, false);
    create_lib_cmake(name);

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
                    "                (automatically includes library.cmake if exists)\n";
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
