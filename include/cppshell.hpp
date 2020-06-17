#ifndef CPPSHELL_CPPSHELL_HPP
#define CPPSHELL_CPPSHELL_HPP

#include <boost/process.hpp>
#include <iostream>
#include <fstream>
#include <vector>

namespace cppshell
{

struct string_out
{
    string_out(std::string &target)
        : target {target}
    {}

    string_out(string_out &&) = default;

    std::string &target;
    boost::process::ipstream stream;
};

struct std_err
{
    explicit std_err(const char *path)
        : path {path}
    {}

    std::string path;
};

struct std_out
{
    explicit std_out(const char *path)
        : path {path}
    {}

    std::string path;
};

struct pipe_in {
    explicit pipe_in(boost::process::pstream &stream)
        : stream {stream}
    {}

    boost::process::pstream &stream;
};

struct pipe_out {
    explicit pipe_out(boost::process::pstream &stream)
        : stream {stream}
    {}

    boost::process::pstream &stream;
};

struct sync_exec
{
    template<class... Args>
    auto run(Args&&... args)
    {
        return boost::process::system(std::forward<Args>(args)...);
    }
};

struct parallel_exec
{
    parallel_exec(parallel_exec &&) = default;
    parallel_exec(boost::process::child& child)
        : child {child}
    {}

    boost::process::child& child;

    template<class... Args>
    auto run(Args&&... args)
    {
        child = boost::process::child(std::forward<Args>(args)...);
        return 0;
    }
};

template<class Exec, class... Args>
struct process_desc
{
    // create process with given command line
    process_desc(std::string const& executable, Exec exec = Exec{})
        : args{executable}
        , executor {std::move(exec)}
    {}

    // TODO: verify IncomingArgs vs Args. IncomingArgs needs to be
    // subset of Args. Otherwise nasty compiler error will appear. :-)
    template<class... IncomingArgs, class Append>
    process_desc(process_desc<Exec, IncomingArgs...> &&pr, Append&& app)
        : args{std::tuple_cat(pr.args, std::make_tuple(std::move(app)))}
    {
        pr.execute = false;
    }

    // TODO: verify IncomingArgs vs Args. IncomingArgs needs to be
    // subset of Args. Otherwise nasty compiler error will appear. :-)
    template<class OldExec, class... IncomingArgs, class Append>
    process_desc(process_desc<OldExec, IncomingArgs...> &&pr, Append&& app, Exec exec = Exec())
        : args{std::tuple_cat(pr.args, std::tuple(app))}
        , executor {std::move(exec)}
    {
        pr.execute = false;
    }

    template<class T> void drain(T&){}
    void drain(string_out &o)
    {
        o.target = std::string(std::istreambuf_iterator<char>(o.stream), {});
    }

    template<std::size_t... I>
    auto run(std::index_sequence<I...>)
    {
        auto result = executor.run(extract(std::get<I>(args))...);
        (drain(std::get<I>(args)), ...);
        return result;
    }

    template<class A>
    auto extract(A& arg)
    {
        return arg;
    }

    auto extract(std_err e)
    {
        return boost::process::std_err = e.path;
    }

    auto extract(std_out e)
    {
        return boost::process::std_out = e.path;
    }

    auto extract(pipe_out p)
    {
        return boost::process::std_out = p.stream;
    }

    auto extract(pipe_in p)
    {
        return boost::process::std_in = p.stream;
    }

    auto extract(string_out &p)
    {
        return boost::process::std_out = p.stream;
    }

    ~process_desc()
    {
        if (execute) {
            run(std::make_index_sequence<std::tuple_size_v<decltype(args)>>{});
        }
    }

    operator int()
    {
        if (execute) {
            execute = false;
            return run(std::make_index_sequence<std::tuple_size_v<decltype(args)>>{});
        }

        return -1;
    }

    operator bool()
    {
        return static_cast<int>(*this) == 0;
    }

    bool execute = true;
    std::tuple<std::string, Args...> args;
    Exec executor;
};

template<class Exec, class Append, class... Args>
auto append(process_desc<Exec, Args...> &&source, Append app)
{
    return process_desc<Exec, Args..., Append>(
        std::forward<process_desc<Exec, Args...>>(source), app
    );
}

process_desc<sync_exec> operator ""_e(const char *name, std::size_t)
{
    return process_desc<sync_exec>{name};
}

auto operator ""_err(const char *name, std::size_t)
{
    return std_err{name};
}

template<class E, class... Args>
auto operator>(process_desc<E, Args...> &&p, const char* out)
{
    return append(std::forward<process_desc<E, Args...>>(p), std_out{out});
}

template<class E, class... Args>
auto operator>(process_desc<E, Args...> &&p, std_out out)
{
    return append(std::forward<process_desc<E, Args...>>(p), out);
}

template<class E, class... Args>
auto operator>(process_desc<E, Args...> &&p, std_err out)
{
    return append(std::forward<process_desc<E, Args...>>(p), out);
}

template<class E, class... Args>
auto operator>(process_desc<E, Args...> &&p, std::string& out)
{
    return append(std::forward<process_desc<E, Args...>>(p), string_out{out});
}

template<class First, class Second>
struct piped_executables
{
    boost::process::pstream     stream;
    boost::process::child       first_child;
    boost::process::child       second_child;

    template<class E1, class... Args1,
             class E2, class... Args2>
    piped_executables(process_desc<E1, Args1...> &&first, process_desc<E2, Args2...> &&second)
        : stream {}
    {
        First f{
            std::forward<process_desc<E1, Args1...>>(first),
            pipe_out{stream},
            parallel_exec{first_child}
        };

        Second s{
            std::forward<process_desc<E2, Args2...>>(second),
            pipe_in{stream},
            parallel_exec{second_child}
        };
    }

    piped_executables(piped_executables &&) = default;

    ~piped_executables()
    {
        first_child.wait();
        second_child.wait();
    }
};

template<class E1, class... Args1,
         class E2, class... Args2>
auto operator|(process_desc<E1, Args1...> &&p1,
               process_desc<E2, Args2...> &&p2)
{
    piped_executables<
        process_desc<parallel_exec, Args1..., pipe_out>,
        process_desc<parallel_exec, Args2..., pipe_in>> piped(
            std::forward<process_desc<E1, Args1...>>(p1),
            std::forward<process_desc<E2, Args2...>>(p2));
}

} // namespace cppshell

#endif
