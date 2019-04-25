/*
CLI Macros utility functions

(c) Elias Bachaalany <elias.bachaalany@gmail.com>
*/

//-------------------------------------------------------------------------
// Utility function similar to Python's re.sub().
// Based on https://stackoverflow.com/a/37516316
namespace std
{
    template<class BidirIt, class Traits, class CharT, class UnaryFunction>
    std::basic_string<CharT> regex_replace(BidirIt first, BidirIt last,
        const std::basic_regex<CharT, Traits> &re, UnaryFunction f)
    {
        std::basic_string<CharT> s;

        typename std::match_results<BidirIt>::difference_type positionOfLastMatch = 0;
        auto endOfLastMatch = first;

        auto callback = [&](const std::match_results<BidirIt> &match)
        {
            auto positionOfThisMatch = match.position(0);
            auto diff = positionOfThisMatch - positionOfLastMatch;

            auto startOfThisMatch = endOfLastMatch;
            std::advance(startOfThisMatch, diff);

            s.append(endOfLastMatch, startOfThisMatch);
            s.append(f(match));

            auto lengthOfMatch = match.length(0);

            positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

            endOfLastMatch = startOfThisMatch;
            std::advance(endOfLastMatch, lengthOfMatch);
        };

        std::regex_iterator<BidirIt> begin(first, last, re), end;
        std::for_each(begin, end, callback);

        s.append(endOfLastMatch, last);

        return s;
    }

    template<class Traits, class CharT, class UnaryFunction>
    std::string regex_replace(const std::string &s,
        const std::basic_regex<CharT, Traits> &re, UnaryFunction f)
    {
        return regex_replace(s.cbegin(), s.cend(), re, f);
    }
}

//-------------------------------------------------------------------------
// [Un]install a CLI (asynchronously) using an UI request
void request_install_cli(const cli_t *cli, bool install)
{
    extern bool g_b_ignore_ui_notification;
    class cli_install_req_t: public ui_request_t
    {
        const cli_t *cli;
        bool install;
    public:
        cli_install_req_t(const cli_t *cli, bool install) : cli(cli), install(install)
        {
        }

        virtual bool idaapi run()
        {
            g_b_ignore_ui_notification = true;
            if (install)
                install_command_interpreter(cli);
            else
                remove_command_interpreter(cli);
            g_b_ignore_ui_notification = false;

            return false;
        }
    };

    execute_ui_requests(
        new cli_install_req_t(cli, install),
        nullptr);
}

//-------------------------------------------------------------------------
// Finds the Python external language object (once and caches it)
extlang_t *pylang()
{
    struct find_python: extlang_visitor_t
    {
        extlang_t **pylang;
        virtual ssize_t idaapi visit_extlang(extlang_t *extlang) override
        {
            if (streq(extlang->fileext, "py"))
            {
                *pylang = extlang;
                return 1;
            }
            return 0;
        }

        find_python(extlang_t **pylang): pylang(pylang)
        {
            for_all_extlangs(*this, false);
        }
    };

    static extlang_t *s_pylang = nullptr;
    if (s_pylang == nullptr)
        find_python{ &s_pylang };

    return s_pylang;
}

//-------------------------------------------------------------------------
// Utility class to replace macros
class macro_replacer_t
{
public:
    using repl_func_t = std::function<std::string(std::string)>;

private:
    static std::regex RE_EVAL;
    std::regex re_replace;

    struct LongerPatternSort
    {
        bool operator()(const std::string& lhs, const std::string& rhs) const
        {
            if (lhs.size() > rhs.size())
                return true;
            else if (lhs.size() < rhs.size())
                return false;
            else
                return lhs < rhs;
        }
    };
    std::map<std::string, std::string, LongerPatternSort> replace_map;

    repl_func_t m_repl_func;

public:
    macro_replacer_t(repl_func_t repl_func) : m_repl_func(repl_func) {}

    std::string operator()(std::string text)
    {
        if (!replace_map.empty())
            text = std::regex_replace(text, re_replace, [this](auto &m) { return replace_map[m.str(0)]; });

        return std::regex_replace(text, RE_EVAL, [this](auto &m) { return m_repl_func(m.str(1)); });
    }

    // Similar to Python's "re.escape()"
    static std::string escape_re(const std::string re_text)
    {
        std::string out;
        out.reserve(re_text.size() * 2);
        for (auto ch: re_text)
        {
            if (isalnum(ch))
            {
                out += ch;
                continue;
            }
            else if (ch == 0)
            {
                out += "\\x0";
            }
            else
            {
                out += '\\';
                out += ch;
            }
        }
        return out;
    }

    void begin_update()
    {
        replace_map.clear();
    }

    void update(std::string macro, std::string expr)
    {
        replace_map[macro] = expr;
    }

    void end_update()
    {
        if (replace_map.empty())
            return;

        std::string re_str;
        for (auto &kv: replace_map)
        {
            re_str.append(escape_re(kv.first));
            re_str.append("|");
        }
        // Get rid of the trailing '|'
        re_str.pop_back();

        // Form the single regular expression
        re_replace = re_str;
    }
};
std::regex macro_replacer_t::RE_EVAL = std::regex(R"(\$\{(.+?)\}\$)");
