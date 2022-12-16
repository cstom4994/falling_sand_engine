// Metadot muscript is enhanced based on yuescript modification
// Metadot code copyright(c) 2022, KaoruXun All rights reserved.
// Yuescript code by Jin Li licensed under the MIT License
// Link to https://github.com/pigpigyyy/Yuescript

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace mu {

    extern const std::string_view version;
    extern const std::string_view extension;

    using Options = std::unordered_map<std::string, std::string>;

    struct MuConfig
    {
        bool lintGlobalVariable = false;
        bool implicitReturnRoot = true;
        bool reserveLineNumber = true;
        bool useSpaceOverTab = false;
        bool exporting = false;
        bool profiling = false;
        int lineOffset = 0;
        std::string module;
        Options options;
    };

    struct GlobalVar
    {
        std::string name;
        int line;
        int col;
    };

    using GlobalVars = std::vector<GlobalVar>;

    struct CompileInfo
    {
        std::string codes;
        std::string error;
        std::unique_ptr<GlobalVars> globals;
        std::unique_ptr<Options> options;
        double parseTime;
        double compileTime;
    };

    class MuCompilerImpl;

    class MuCompiler {
    public:
        MuCompiler(void *luaState = nullptr, const std::function<void(void *)> &luaOpen = nullptr,
                   bool sameModule = false);
        virtual ~MuCompiler();
        CompileInfo compile(std::string_view codes, const MuConfig &config = {});

    private:
        std::unique_ptr<MuCompilerImpl> _compiler;
    };

}// namespace mu
