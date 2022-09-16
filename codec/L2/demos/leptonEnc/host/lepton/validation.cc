#include "../vp8/util/memory.hh"
#ifdef _WIN32
#include <io.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <signal.h>
#include "../vp8/util/nd_array.hh"
#include "../io/MuxReader.hh"
#include "../io/ioutil.hh"
#include "validation.hh"
ValidationContinuation validateAndCompress(int* reader,
                                           int* writer,
                                           Sirikata::Array1d<uint8_t, 2> header,
                                           size_t start_byte,
                                           size_t end_byte,
                                           ExitCode* validation_exit_code,
                                           Sirikata::MuxReader::ResizableByteBuffer* lepton_data,
                                           int argc,
                                           const char** argv,
                                           bool is_socket) {
#ifdef _WIN32
    std::vector<const char*> args;
    args.push_back(argv[0]);
    args.push_back("-skiproundtrip");
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-' && strcmp("argv[i]", "-") && strstr(argv[i], "-validat") != argv[i] &&
            strstr(argv[i], "-verif") != argv[i] && strstr(argv[i], "-socket") != argv[i] &&
            strstr(argv[i], "-fork") != argv[i] && strstr(argv[i], "-listen") != argv[i] &&
            strstr(argv[i], "-roundtrip") != argv[i]) {
            args.push_back(argv[i]);
        }
    }
    args.push_back("-"); // read from stdin, write to stdout
    // args.push_back("/Users/daniel/Source/Repos/lepton/images/iphone.jpg");
    // args.push_back("/Users/daniel/Source/Repos/lepton/test.lep");
    auto encode_pipes = IOUtil::start_subprocess(args.size(), &args[0], false);
    lepton_data->reserve(4096 * 1024);
#else
    int jpeg_input_pipes[2] = {-1, -1};
    int lepton_output_pipes[2] = {-1, -1};
    int lepton_roundtrip_send[2] = {-1, -1};
    int jpeg_roundtrip_recv[2] = {-1, -1};
    // int err;
    while (pipe(jpeg_input_pipes) < 0 && errno == EINTR) {
    }
    while (pipe(lepton_output_pipes) < 0 && errno == EINTR) {
    }
    pid_t encode_pid;
    pid_t decode_pid;
    if ((encode_pid = fork()) == 0) { // could also fork/exec here
        // not yet open -- we will exit before accessed while(close(*fwriter) < 0 && errno == EINTR){}
        if (*writer != -1 && *writer != *reader) {
            while (close(*writer) < 0 && errno == EINTR) {
            }
        }
        while (close(*reader) < 0 && errno == EINTR) {
        }
        *reader = jpeg_input_pipes[0];
        *writer = lepton_output_pipes[1];
        while (close(jpeg_input_pipes[1]) < 0 && errno == EINTR) {
        }
        while (close(lepton_output_pipes[0]) < 0 && errno == EINTR) {
        }
        return ValidationContinuation::CONTINUE_AS_JPEG;
    }
    while (close(jpeg_input_pipes[0]) < 0 && errno == EINTR) {
    }
    while (close(lepton_output_pipes[1]) < 0 && errno == EINTR) {
    }

    while (pipe(lepton_roundtrip_send) < 0 && errno == EINTR) {
    }
    while (pipe(jpeg_roundtrip_recv) < 0 && errno == EINTR) {
    }
    // we wanna fork the decode here before we allocate 4096 * 1024 bytes here
    if ((decode_pid = fork()) == 0) { // could also fork/exec here
        if (*writer != -1 && *writer != *reader) {
            while (close(*writer) < 0 && errno == EINTR) {
            }
        }

        while (close(*reader) < 0 && errno == EINTR) {
        }
        // not yet open -- we will exit before accessed while(close(*fwriter) < 0 && errno == EINTR){}
        while (close(jpeg_input_pipes[1]) < 0 && errno == EINTR) {
        }
        while (close(lepton_output_pipes[0]) < 0 && errno == EINTR) {
        }

        *reader = lepton_roundtrip_send[0];
        *writer = jpeg_roundtrip_recv[1];
        while (close(lepton_roundtrip_send[1]) < 0 && errno == EINTR) {
        }
        while (close(jpeg_roundtrip_recv[0]) < 0 && errno == EINTR) {
        }

        return ValidationContinuation::CONTINUE_AS_LEPTON;
    }
    while (close(lepton_roundtrip_send[0]) < 0 && errno == EINTR) {
    }
    while (close(jpeg_roundtrip_recv[1]) < 0 && errno == EINTR) {
    }

    lepton_data->reserve(4096 * 1024);
    int status = 0;
    while (waitpid(encode_pid, &status, 0) < 0 && errno == EINTR) {
    } // wait on encode
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            exit(exit_code);
        }
    } else if (WIFSIGNALED(status)) {
        raise(WTERMSIG(status));
    }
    size_t roundtrip_size = 0;

    status = 0;
    while (waitpid(decode_pid, &status, 0) < 0 && errno == EINTR) {
    } // wait on encode
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            exit(exit_code);
        }
    } else if (WIFSIGNALED(status)) {
        raise(WTERMSIG(status));
    }
#endif
    *validation_exit_code = ExitCode::SUCCESS;
    return ValidationContinuation::ROUNDTRIP_OK;
}
