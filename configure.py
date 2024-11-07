from __future__ import annotations

from subprocess import run, PIPE
import os
import sys
import atexit
import shutil


GEN: list[str] = []


IS_MSYS2 = 'MSYSTEM' in os.environ and os.environ.get('MSYSTEM') in {'MSYS', 'MINGW64', 'MINGW32', "UCRT64", "UCRT32"}


try:
    os.mkdir("./configure")
except:
    pass
os.chdir("./configure")


def check_bin(name: str):
    print(f"Checking binary: {name.ljust(60, '.')}...", end="")
    if shutil.which(name) == None:
        print("fail")
        exit(1)
    print("ok")


def check(head: str | None, feat: str, headers: list[str], example: str, mandatory: bool = False, link: list[str] = []):
    """
    head: str = The macro name, i.e. HAVE_STRTOIMAX
    feat: str = The feature name, i.e. "Has strtoimax"
    headers: list[str] = The headers required
    example: str = The code within main(), return 0 on success, 1 on failure
    """
    print(f"Checking with c: {feat.ljust(60, '.')}...", end="")

    try:
        code = f"""\
{'\n'.join([f'#include <{i}>' for i in headers])}

int main(int argc, char **argv) {'{'}
{example}
{'}'}"""
        executable_path = "./test"
        if IS_MSYS2:
            executable_path = "./test.exe"

        with open("./test.c", "w") as c:
            c.write(code)

        link_flags = []
        if len(link):
            if IS_MSYS2:
                link_flags += list(map(lambda l: "-l"+l, link))
            else:
                link_flags += list(map(lambda l: "-l"+l, link))
        print(["gcc", "-xc", "./test.c", "-o", executable_path] + link_flags)

        gcc = run(["gcc", "-xc", "./test.c", "-o", executable_path] + link_flags,
                  stdout=PIPE, stderr=PIPE)
        if gcc.returncode != 0:
            os.remove("./test.c")
            print(f"gcc fail")
            if mandatory:
                print("MANDATORY CHECK FAILED")
                print(gcc.stderr.decode('utf-8'))
                exit(1)
            return

        os.remove("./test.c")

        ret = run(executable_path, stderr=PIPE, stdout=PIPE)
        try:
            os.remove(executable_path)
        except:
            pass

        success = ret.returncode == 0
        if success:
            if head:
                GEN.append(f"#define {head}")
            print("yes")
        else:
            print("no")
            if mandatory:
                print("MANDATORY CHECK FAILED")
                exit(1)
            if head:
                GEN.append(f"//#define {head}")
        return
    except SystemExit as e:
        raise e
    except BaseException as e:
        pass

    try:
        os.remove("./test.c")
    except:
        pass
    try:
        os.remove("./test")
        os.remove("./test.exe")
    except:
        pass
    print("error")
    if mandatory:
        print("MANDATORY CHECK FAILED")
        exit(1)


def check_header(head: str | None, header: str, mandatory: bool = False):
    """
    head: str = The macro name, i.e. HAVE_STRTOIMAX
    feat: str = The feature name, i.e. "Has strtoimax"
    headers: list[str] = The headers required
    example: str = The code within main(), return 0 on success, 1 on failure
    """
    print(f"Checking header: {header.ljust(60, ".")}...", end="")

    try:
        code = f"""\
#include <{header}>

int main(int argc, char **argv) {"{"}
return 0;
{"}"}\
    """

        with open("./test.c", "w") as c:
            c.write(code)

        gcc = run(["gcc", "./test.c", "-o", "./test"],
                  stdout=PIPE, stderr=PIPE)
        if gcc.returncode != 0:
            os.remove("./test.c")
            print("can't import")
            if head:
                GEN.append(f"//#define {head}")
            if mandatory:
                print("MANDATORY CHECK FAILED")
                print(gcc.stderr.decode('utf-8'))
                exit(1)
            return

        print("exists")
        if head:
            GEN.append(f"#define {head}")
    except:
        print("fail")
        pass


def define_stdout(head: str | None, name: str, cmd: str, mandatory: bool = False):
    print(f"Defining       : {name.ljust(60, ".")}...", end="")
    out = run(["bash", "-c", cmd], stdout=PIPE, stderr=PIPE)
    print(out.stdout.decode("utf-8"))
    if out.returncode == 0:
        if head:
            GEN.append(f"#define {head} {out.stdout.decode('utf-8').removesuffix('\n')!r}")
        print("success")
    else:
        if head:
            GEN.append(f"//#define {head}")
        print("fail")


def cleanup_file(path: str):
    try:
        os.remove(path)
    except:
        pass


def try_cleanup():
    try:
        os.remove("./temp")
    except:
        pass
    try:
        os.remove("./test.c")
    except:
        pass
    try:
        os.remove("./test")
    except:
        pass

    try:
        os.chdir("..")
        os.rmdir("./configure")
    except:
        pass


atexit.register(try_cleanup)


###########################
## The checks start here ##
###########################

check_bin("rm")
check_bin("cp")
check_bin("mkdir")
check_bin("touch")
check_bin("gcc")

REQUIRED_HEADERS = [
    "limits.h",
    "stddef.h",
    "stdint.h",
    "stdio.h",
    "stdlib.h",
    "string.h",
    "unistd.h",
    "fcntl.h",
    "curl/curl.h",
    "curl/urlapi.h",
]
for head in REQUIRED_HEADERS:
    check_header(None, head, True)


check(None, "can find filesystem functions", ["string.h", "stdlib.h", "stdio.h"], """\
void *ptr_a = (void*)&fopen;
if (ptr_a == NULL) return 1;
void *ptr_b = (void*)&fclose;
if (ptr_b == NULL) return 1;
void *ptr_c = (void*)&fwrite;
if (ptr_c == NULL) return 1;
void *ptr_d = (void*)&fread;
if (ptr_d == NULL) return 1;
return 0;\
""", True)
check(None, "can do filesystem operations", ["string.h", "stdlib.h", "stdio.h"], """\
char *temp_path = "./temp";
FILE *f = fopen(temp_path, "w");
if (f == NULL) return 1;
char data[] = "This is an example bit of text";
size_t s = fwrite(data, sizeof(data), 1, f);
if (s != 1) return 1;
fclose(f);
f = fopen(temp_path, "r");
char buf[sizeof(data)];
s = fread(buf, sizeof(buf), 1, f);
if (s != 1) return 1;
for (int i = 0; i < sizeof(data); i += 1) {
      if (data[i] != buf[i]) return 1;
}
fclose(f);
return 0;\
""", True)
cleanup_file("./temp")

check("HAVE_FPRINTF_STDERR", "fprintf can be used for stderr", ["inttypes.h", "stdlib.h", "stdio.h"], """\
int amount = fprintf(stderr, "Hello!");
if (amount != sizeof("Hello")) return 1;
return 0;\
""", True)
cleanup_file("./temp")

check(None, "strlen exists and functions", ["stdlib.h", "string.h"], """\
char test[] = "123456789";
int len = strlen(test);
if (len != sizeof(test) - 1) return 1;
return 0;\
""", True)
check(None, "strcmp exists and functions", ["stdlib.h", "string.h"], """\
if (0 != strcmp("FOO", "FOO")) return 1;
return 0;\
""", True)

check(None, "memcpy exists and functions", ["stdlib.h", "string.h"], """\
int a = 20;
int b = 10;
memcpy(&a, &b, sizeof(int));
if (a != b) return 1;
return 0;\
""", True)
check(None, "memset exists and functions", ["stdlib.h", "string.h"], """\
unsigned char a[2] = { 1, 3 };
memset(a, 12, 2);
if (a[0] != 12 || a[1] != 12) return 1;
return 0;\
""", True)
check(None, "malloc can be called", ["stdlib.h", "string.h"], """\
unsigned char* data = malloc(sizeof(unsigned char)*512);
for (unsigned char i = 0; i < 255; i += 1) {
    memset(data, i, 512);
    for (int b = 0; b < 512; b += 1) {
        if (data[b] != (unsigned char)i) return 1;
    }
}
free(data);
return 0;\
""", True)

check(None, "strtoq can be used", ["inttypes.h", "stdlib.h"], """\
char *test = "38";
char *endptr;
long res = strtoq(test, &endptr, 10);
if (endptr != &test[2]) return 1;
if (res != 38) return 2;
return 0;\
""")
check("HAVE_STRTOL", "strtol can be used", ["inttypes.h", "stdlib.h"], """\
char *test = "38";
char *endptr;
long res = strtol(test, &endptr, 10);
if (endptr != &test[2]) return 1;
if (res != 38) return 2;
return 0;\
""", True)
check("HAVE_STRTOIMAX", "strtoimax can be used", ["inttypes.h", "string.h"], """\
char *test = "38";
char *endptr;
intmax_t res = strtoimax(test, &endptr, 10);
if (endptr != &test[2]) return 1;
if (res != 38) return 2;
return 0;\
""")

check(None, "Can link with curl", ["curl/curl.h"], """\
char *ver = curl_version();
printf(ver);
return 0;
""", True, link=["curl"])

define_stdout("COMPILER_ARCH", "architecture", "uname -m")


with open("../src/config.h", "w") as h:
    h.write("/* THIS FILE IS AUTO-GENERATED, RUN CONFIGURE.PY TO GENERATE */\n" +
            "\n".join(GEN)+"\n")

print("CONFIGURE COMPLETE! You should be ready to build")
