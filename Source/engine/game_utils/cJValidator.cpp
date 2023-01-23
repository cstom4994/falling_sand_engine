#include "cJValidator.h"

#include <cmath>
#include <cstdlib>

namespace cjsonpp {

/*
##### 数据类型

​	int,float,bool,string

##### 是否可选，默认为必选

​	optional 可省略为opt

##### 范围区间表示

​	(1,100) 表示 1 < x < 100
​	[1,100) 表示 1 <= x < 100
​	(1,100] 表示 1 < x <= 100
​	[1,100] 表示 1 <= x <= 100
​	{1,2,3,4,5} 表示在集合内
    !{1,2,3} 表示不在集合内

##### 示例如下：

1. int
2. int [1,100]
3. optioal int (1,100)
4. float (0,1)
5. string [10,20]  表示字符串长度范围
6. int >0
*/

#define S_OPTINAL "optinal"
#define S_INT "int"
#define S_FLOAT "float"
#define S_BOOL "bool"
#define S_STRING "string"

#define CSTR_LEN(m) (sizeof(m) - 1)

const char* skipSpace(const char* str) {
    while (*str) {
        if (*str == ' ' || *str == '\t') {
            ++str;
        } else {
            break;
        }
    }
    return str;
}

inline bool isEnd(char ch) { return ch == '\0' || ch == ' '; }

inline bool equare(double d1, double d2) { return fabs(d1 - d2) < 1e-6; }

// 字符串比较，遇到空格字符会结束
int strCmp(const char* dest, const char* source) {
    while (*dest && *source && *dest != ' ' && *source != ' ' && (*dest == *source)) {
        dest++;
        source++;
    }

    if (isEnd(*dest) && isEnd(*source)) {
        return 0;
    } else {
        return *dest - *source;
    }
}

// 检测范围
bool checkRange(const Json& value, const char* str) {
    switch (str[0]) {
        case '(':
        case '[': {
            char* endstr;
            char start = str[0];
            str = skipSpace(str + 1);
            double a = strtod(str, &endstr);
            str = skipSpace(endstr + 1);
            double b = strtod(str, &endstr);
            str = skipSpace(endstr);

            // 检测最小范围
            if (value.isNumber()) {
                if (start == '[') {
                    if (value.to<double>() < a) {
                        return false;
                    }
                } else {
                    if (value.to<double>() <= a) return false;
                }
            } else if (value.isString()) {
                if (start == '[') {
                    if (value.size() < a) return false;
                } else {
                    if (value.size() <= a) return false;
                }
            }

            // 没有结束符，不检测最大范围
            if (*str != ']' && *str != ')') {
                return true;
            }

            // 检测最大范围
            if (value.isNumber()) {
                if (*str == ']') {
                    return value.to<double>() <= b;
                } else {
                    return value.to<double>() < b;
                }
            } else if (value.isString()) {
                if (*str == ']') {
                    return value.size() <= b;
                } else {
                    return value.size() < b;
                }
            }
            break;
        }
        case '{': {
            // 集合
            char* endstr;
            str = skipSpace(str + 1);
            double num;
            if (value.isString())
                num = value.size();
            else
                num = value.to<double>();

            while (*str) {
                double a = strtod(str, &endstr);
                if (equare(a, num)) {
                    return true;
                }
                str = skipSpace(endstr + 1);
            }

            // 不在集合内返回
            return false;
        }
        case '!': {
            // 不在集合内
            if (str[1] == '{') {
                char* endstr;
                str = skipSpace(str + 2);
                double num;
                if (value.isString())
                    num = value.size();
                else
                    num = value.to<double>();
                while (*str) {
                    double a = strtod(str, &endstr);
                    if (equare(a, num)) {
                        return false;
                    }
                    str = skipSpace(endstr + 1);
                }
            }
            break;
        }
        case '>':
            if (str[1] == '=') {
                str = skipSpace(str + 2);
                double a = atof(str);
                if (value.isNumber()) {
                    return value.to<double>() >= a;
                } else if (value.isString()) {
                    return value.size() >= a;
                }
            } else {
                str = skipSpace(str + 1);
                double a = atof(str);
                if (value.isNumber()) {
                    return value.to<double>() > a;
                } else if (value.isString()) {
                    return value.size() > a;
                }
            }
            break;
        case '<':
            if (str[1] == '=') {
                str = skipSpace(str + 2);
                double a = atof(str);
                if (value.isNumber()) {
                    return value.to<double>() <= a;
                } else if (value.isString()) {
                    return value.size() <= a;
                }
            } else {
                str = skipSpace(str + 1);
                double a = atof(str);
                if (value.isNumber()) {
                    return value.to<double>() < a;
                } else if (value.isString()) {
                    return value.size() < a;
                }
            }
            break;
        case '=':
            if (str[1] == '=') {
                str = skipSpace(str + 2);
                double a = atof(str);
                if (value.isNumber()) {
                    return equare(value.to<double>(), a);
                } else if (value.isString()) {
                    return equare(value.size(), a);
                }
            } else {
                str = skipSpace(str + 1);
                double a = atof(str);
                if (value.isNumber()) {
                    return equare(value.to<double>(), a);
                } else if (value.isString()) {
                    return equare(value.size(), a);
                }
            }
            break;
        default:
            break;
    }
    return true;
}

inline bool isOptional(const char* opt) {
    opt = skipSpace(opt);
    return strCmp(opt, S_OPTINAL) == 0;
}

bool Validator::verifyValue(const Json& value, const char* opt) {
    opt = skipSpace(opt);

    // 解析optinal
    bool optinal = strCmp(opt, S_OPTINAL) == 0;
    if (optinal) {
        if (!value.valied()) {
            return true;
        }
        opt += CSTR_LEN(S_OPTINAL);
        opt = skipSpace(opt);
    }

    // 解析type
    if (strCmp(opt, S_INT) == 0) {
        if (!value.isNumber()) {
            error_ = std::string(value.name()) + " must int";
            return false;
        }
        opt += CSTR_LEN(S_INT);
    } else if (strCmp(opt, S_FLOAT) == 0) {
        if (!value.isNumber()) {
            error_ = std::string(value.name()) + " must float";
            return false;
        }
        opt += CSTR_LEN(S_FLOAT);
    } else if (strCmp(opt, S_BOOL) == 0) {
        if (!value.isBool()) {
            error_ = std::string(value.name()) + " must bool";
            return false;
        }
        opt += CSTR_LEN(S_BOOL);
    } else if (strCmp(opt, S_STRING) == 0) {
        if (!value.isString()) {
            error_ = std::string(value.name()) + " must string";
            return false;
        }
        opt += CSTR_LEN(S_STRING);
    }
    opt = skipSpace(opt);

    // 解析范围
    if (!checkRange(value, opt)) {
        error_ = std::string(value.name()) + " not in " + opt;
        return false;
    }
    return true;
}

bool Validator::validate(const Json& data, const Json& schema) {
    if (!data.valied()) {
        return false;
    }
    switch (schema.type()) {
        case Json::kArray:
            if (!data.isArray()) {
                error_ = std::string(data.name()) + " must array";
                return false;
            }

            {
                Json s = schema.at(0);
                if (s) {
                    for (auto item : data) {
                        if (!validate(item, s)) {
                            return false;
                        }
                    }
                }
            }
            break;
        case Json::kObject:
            if (!data.isObject()) {
                error_ = std::string(data.name()) + " must object";
                return false;
            }
            for (auto itr = schema.begin(); itr != schema.end(); ++itr) {
                Json child = data.at(itr.key());
                if (child.valied()) {
                    if (!validate(child, *itr)) return false;
                } else {
                    if (!isOptional((*itr).to<const char*>())) {
                        error_ = std::string(itr.key()) + " is not exist";
                        return false;
                    }
                }
            }
            break;
        case Json::kString:
            if (!verifyValue(data, schema.to<const char*>())) {
                return false;
            }
            break;
        default:
            error_ = "invalied schema value";
            return false;
    }
    return true;
}

}  // namespace cjsonpp
