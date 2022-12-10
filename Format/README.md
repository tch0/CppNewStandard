# 格式化库

GCC12.1.0尚不支持，使用[`fmt`库](https://github.com/fmtlib/fmt)作为代替。
- 定义头文件`<format>`：
```C++
#pragma once
#define FMT_HEADER_ONLY
#include <fmt/format.h>
namespace std
{
    using fmt::format;
    using fmt::format_error;
    using fmt::formatter;
}
```
- 并将其目录加到编译器头文件搜索路径，`fmt`库的`include`目录同样要加。
- 然后就可以像他们就在标准库里那样用就可以了，等到编译器支持后，即可移除这些措施。

## 格式化

细节见[标准格式说明](https://zh.cppreference.com/w/cpp/utility/format/formatter#.E6.A0.87.E5.87.86.E6.A0.BC.E5.BC.8F.E8.AF.B4.E6.98.8E)。

使用：`result = std::format(format-str, args...)`

格式字符串：`{n:format-specifier}`：
- 其中`n`为从0开始的序号，可以省略。
- 省略时为自动索引，按顺序依次填充参数。
- 不省略时为手动索引，格式化对应索引的参数。
- 不允许混合自动和手动索引。

格式说明符`[[fill] align] [sign] [#] [0] [width] [.precision] [type]`：
- `fill`填充字符：单个字符，确保输出中的字段达到`[width]`的最小宽度。
- `align`对齐：`<^>`分别表示左对齐（非整数和浮点数默认）、居中对齐、右对齐（整数浮点数默认）。如果未指定宽度，则`[fill]align`无效。
- `wdith`输出字段宽度：一个值则表示宽度，如果是`{}`则使用下一个字段作为宽度，如果是`{index}`则使用索引字段作为输出宽度。
- `sign`符号：`-/+/space`分别表示只显示负号（默认）、显示正号和负号、对负数使用负号整数使用空格。
- `#`：启用备用格式规则。如果是整数会在格式化数字前插入`0x 0X 0b 0B 0`。如果是浮点数则备用格式始终输出十进制分隔符，即是后面没有数字。
- `type`类型：
    - 整型：`bBdoxX`分别表示二进制、大写二进制（只备用格式使用`0B`）、十进制（未指定的默认）
    八进制、十六进制、大写十六进制。
    - 浮点：
        - `e E`：用`e E`表示的科学计数法，按照给定精度或者默认精度6格式化。
        - `f F`：固定表示法，按照给定精度或者6。
        - `g G`：以`e E`表示指数的通用表示法，按照给定精度或者6格式化。
        - `a A`：小写或者大写的十六进制表示。
    - 布尔：`sbBcdoxX`，其中`s`以字符串形式输出`true/false`（布尔型默认），其他按整数或者字符格式输出。
    - 字符：`cbBdoxX`，其中`c`以字符形式输出（字符型默认），其他按整数格式输出。
    - 字符串：`s`，如果类型未指定，则默认使用`s`。
    - 指针：`p`，`0x`为前缀的十六进制表示法，如果类型未指定，指针默认用`p`。
- `precision`精度：
    - 只用于整数和浮点数。
    - 格式是`.`后跟浮点类型要输出的小数位数，或字符串要输出的字符数。
    - 和`width`一样，可以是另一组花括号，这种时候成为动态精度。
- `0`：
    - 对于数值，填充到格式化结果中以达到`[width]`指定的最小宽度。
    - 插在数值前面，已经`0x 0X 0b ...`后面。
    - 如果指定了对齐则忽略本选项。

错误格式：抛出`std::format_error`，`fmt`库很多时候会直接编不过。

支持自定义类型：特化`std::formatter`，细节略，见[`std::formatter`](https://zh.cppreference.com/w/cpp/utility/format/formatter)。