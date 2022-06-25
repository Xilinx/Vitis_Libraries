#include "../../vp8/util/memory.hh"
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <thread>
#define S_IWUSR 0
#define S_IRUSR 0
#else
#include <sys/select.h>
#endif
#include "Reader.hh"
#include "ioutil.hh"
#ifdef _WIN32
#include <Windows.h>
#include <tchar.h>
#endif
namespace IOUtil {
/*
FileReader * OpenFileOrPipe(const char * filename, int is_pipe, int max_file_size) {
    int fp = 0;
    if (!is_pipe) {
        fp = open(filename, O_RDONLY);
    }
    if (fp >= 0) {
        return new FileReader(fp, max_file_size);
    }
    return NULL;
}
*/ /*
 FileWriter * OpenWriteFileOrPipe(const char * filename, int is_pipe) {
     int fp = 1;
     if (!is_pipe) {
         fp = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR | S_IRUSR);
     }
     if (fp >= 0) {
         return new FileWriter(fp, !g_use_seccomp);
     }
     return NULL;
 }
 */

FileReader* BindFdToReader(int fd, unsigned int max_file_size, bool is_socket) {
    if (fd >= 0) {
        return new FileReader(fd, max_file_size, is_socket);
    }
    return NULL;
}
FileWriter* BindFdToWriter(int fd, bool is_socket) {
    if (fd >= 0) {
        return new FileWriter(fd, !g_use_seccomp, is_socket);
    }
    return NULL;
}
void send_all_and_close(int fd, const uint8_t* data, size_t data_size) {
    while (data_size > 0) {
        auto ret = write(fd, data, data_size);
        if (ret == 0) {
            break;
        }
        if (ret < 0 && errno == EINTR) {
            continue;
        }
        if (ret < 0) {
            auto local_errno = errno;
            fprintf(stderr, "Send err %d\n", local_errno);
            custom_exit(ExitCode::SHORT_READ);
        }
        data += ret;
        data_size -= ret;
    }
    while (close(fd) == -1 && errno == EINTR) {
    }
}
void discard_stderr(int fd) {
    char buffer[4097];
    buffer[sizeof(buffer) - 1] = '\0';
    while (true) {
        auto del = read(fd, buffer, sizeof(buffer) - 1);
        if (del <= 0) {
            if (del < 0 && errno == EINTR) {
                continue;
            }
            break;
        }
        buffer[del] = '\0';
        fprintf(stderr, "%s", buffer);
    }
}
SubprocessConnection start_subprocess(int argc, const char** argv, bool pipe_stderr) {
    SubprocessConnection retval;
    memset(&retval, 0, sizeof(retval));
#ifdef _WIN32
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    HANDLE hChildStd_IN_Rd;
    HANDLE hChildStd_IN_Wr;

    HANDLE hChildStd_OUT_Rd;
    HANDLE hChildStd_OUT_Wr;

    HANDLE hChildStd_ERR_Rd;
    HANDLE hChildStd_ERR_Wr;
    bool simpler = true;
    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
        custom_exit(ExitCode::OS_ERROR);
    }
    if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        custom_exit(ExitCode::OS_ERROR);
    }
    if (pipe_stderr || !simpler) {
        if (!CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0)) {
            custom_exit(ExitCode::OS_ERROR);
        }
        if (!SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0)) {
            custom_exit(ExitCode::OS_ERROR);
        }
    }
    if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0)) {
        custom_exit(ExitCode::OS_ERROR);
    }
    if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
        custom_exit(ExitCode::OS_ERROR);
    }
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    memset(&siStartInfo, 0, sizeof(siStartInfo));
    memset(&piProcInfo, 0, sizeof(piProcInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);

    if (pipe_stderr || !simpler) {
        siStartInfo.hStdError = hChildStd_ERR_Wr;
    } else {
        siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.hStdInput = hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    std::vector<char> command_line;
    const char* exe_shorthand = "lepton.exe";
    command_line.insert(command_line.end(), exe_shorthand, exe_shorthand + strlen(exe_shorthand));
    for (int i = 1; i < argc; ++i) {
        command_line.push_back(' ');
        command_line.insert(command_line.end(), argv[i], argv[i] + strlen(argv[i]));
    }
    command_line.push_back('\0');
    if (!CreateProcess(argv[0], &command_line[0],
                       NULL, // process security attributes
                       NULL, // primary thread security attributes,
                       TRUE, // handles inherited,
                       0,    // flags,
                       NULL, // use parent environment,
                       NULL, // use current dir,
                       &siStartInfo, &piProcInfo)) {
        fprintf(stderr, "Failed To start subprocess with command line ", command_line);
        custom_exit(ExitCode::OS_ERROR);
    }
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    if (pipe_stderr || !simpler) {
        CloseHandle(hChildStd_ERR_Wr);
        while ((retval.pipe_stderr = _open_osfhandle((intptr_t)hChildStd_ERR_Rd, O_APPEND | O_RDONLY)) == -1 &&
               errno == EINTR) {
        }
    } else {
        retval.pipe_stderr = -1;
    }
    if (simpler == false && !pipe_stderr) {
        std::thread discard_stderr(std::bind(&discard_stderr, retval.pipe_stderr));
        discard_stderr.detach();
        retval.pipe_stderr = -1;
    }
    CloseHandle(hChildStd_OUT_Wr);
    while ((retval.pipe_stdout = _open_osfhandle((intptr_t)hChildStd_OUT_Rd, O_APPEND | O_RDONLY)) == -1 &&
           errno == EINTR) {
    }
    CloseHandle(hChildStd_IN_Rd);
    while ((retval.pipe_stdin = _open_osfhandle((intptr_t)hChildStd_IN_Wr, O_APPEND | O_WRONLY)) == -1 &&
           errno == EINTR) {
    }
#else
    int stdin_pipes[2] = {-1, -1};
    int stdout_pipes[2] = {-1, -1};
    int stderr_pipes[2] = {-1, -1};
    while (pipe(stdin_pipes) < 0 && errno == EINTR) {
    }
    while (pipe(stdout_pipes) < 0 && errno == EINTR) {
    }
    if (pipe_stderr) {
        while (pipe(stderr_pipes) < 0 && errno == EINTR) {
        }
    }
    if ((retval.sub_pid = fork()) == 0) {
        while (close(stdin_pipes[1]) == -1 && errno == EINTR) {
        }
        while (close(stdout_pipes[0]) == -1 && errno == EINTR) {
        }
        if (pipe_stderr) {
            while (close(stderr_pipes[0]) == -1 && errno == EINTR) {
            }
        }
        while (close(0) == -1 && errno == EINTR) {
        }
        while (dup2(stdin_pipes[0], 0) == -1 && errno == EINTR) {
        }

        while (close(1) == -1 && errno == EINTR) {
        }
        while (dup2(stdout_pipes[1], 1) == -1 && errno == EINTR) {
        }
        if (pipe_stderr) {
            while (close(2) == -1 && errno == EINTR) {
            }
            while (dup2(stderr_pipes[1], 2) == -1 && errno == EINTR) {
            }
        }
        std::vector<char*> args(argc + 1);
        for (int i = 0; i < argc; ++i) {
            args[i] = (char*)argv[i];
        }
        args[argc] = NULL;
        execvp(args[0], &args[0]);
    }
    while (close(stdin_pipes[0]) == -1 && errno == EINTR) {
    }
    while (close(stdout_pipes[1]) == -1 && errno == EINTR) {
    }
    if (pipe_stderr) {
        while (close(stderr_pipes[1]) == -1 && errno == EINTR) {
        }
    }
    retval.pipe_stdin = stdin_pipes[1];
    retval.pipe_stdout = stdout_pipes[0];
    retval.pipe_stderr = stderr_pipes[0];
#endif
    return retval;
}
}
