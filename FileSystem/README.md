<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [C++17 filesystem库](#c17-filesystem%E5%BA%93)
  - [一些filesystem库的概念](#%E4%B8%80%E4%BA%9Bfilesystem%E5%BA%93%E7%9A%84%E6%A6%82%E5%BF%B5)
  - [路径](#%E8%B7%AF%E5%BE%84)
  - [其他相关类型](#%E5%85%B6%E4%BB%96%E7%9B%B8%E5%85%B3%E7%B1%BB%E5%9E%8B)
  - [文件类型判断](#%E6%96%87%E4%BB%B6%E7%B1%BB%E5%9E%8B%E5%88%A4%E6%96%AD)
  - [文件操作函数](#%E6%96%87%E4%BB%B6%E6%93%8D%E4%BD%9C%E5%87%BD%E6%95%B0)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# C++17 filesystem库

文档：[Cpp Reference - filesystem](https://en.cppreference.com/w/cpp/filesystem)。

头文件：`<filesystem>`。

命名空间：`std::filesystem`。

## 一些filesystem库的概念

- 文件：一个file system中的对象，可以读取写入，具有名称、属性和以下类型之一。
    - 目录
    - 普通文件
    - 符号链接
    - 其他文件类型：块、字符、FIFO、SOCKET等。
- 文件名：一个表示文件名称的字符串，可使用的字符是限定的（因实现而异）、大小写敏感、长度有限制，在库层面`.`和`..`具有特殊意义，不可以使用。
- 路径：表示一个文件的一个序列，具有可选的根名称`C: //server`（on WIndows），跟可选的根目录`/`（on Unix），然后跟一系列由分隔符（`/ \`）分隔的非零长的名称。
    - 绝对路径：无二义的标识一个文件的路径。
    - 规范路径：路径中不具有`. ..`这种相对路径。
    - 相对路径：以`. ..`开头的路径，与当前工作目录相对的一个路径。

## 路径

路径是整个文件系统库的核心，类型：[`std::filesystem::path`](https://en.cppreference.com/w/cpp/filesystem/path)。
- 路径仅处理句法层面的东西，可以指向不实际存在与文件系统上的文件或者目录（甚至可以包含当前环境中不支持的符号等）。
- 路径名称包含以下语法：
    - **可选的根名称**（root-name）：比如Windows上的`C: //server`这种，一个合法的可能的最长的根名称会被视为最终的根名称（贪心取最长）。
    - **可选的根目录**（root-directory）：如果存在的话，将这个路径标志为绝对路径的一个目录分隔符（比如Unix上的`/`）。如果不存在根目录，也不存在根名称，那么这个路径就会被解析为一个相对路径，那么就需要一个路径作为开始位置来解析这个相对路径。
    - 零个或者多个以下形式：
        - **文件名**（file name）：不包含目录分隔符的一个字符串序列（这个字符串能包含的字符因OS而异）。这个名称可以标识一个文件、一个硬链接、一个符号链接或者一个目录。两个特殊的文件名被识别作为特殊用途：`.`标识当前目录，`..`标识父目录。
        - **目录分隔符**（directory-separator）：一个斜杠`/`或者一个由`path::preferred_separator`提供的一个其他符号。如果这个符号重复，那么会被视为一个单一的分隔符。（比如`/usr////lib`等同于`/usr/lib`）。
- 一个路径可以由以下算法进行规范化（nomalize）：
    - 如果为空，停止。
    - 使用一个`path::preferred_separator`替换每个目录分隔符（可能有多个）。
    - 使用`path::preferred_separator`替换根名称中的每一个`/`。
    - 移除每一个`.`，以及其后紧跟的目录分隔符，即解析`.`。
    - 移除每一个其后紧跟目录分隔符和`..`的文件名，以及其后紧跟的目录分隔符，即解析`..`。
    - 如果有根目录，移除其后紧跟的所有`..`和其后紧跟的目录分隔符。（根目录的父目录还是自己）。
    - 如果最后一个文件名是`..`，移除随后的目录分隔符。
    - 如果路径为空，添加一个`.`（`./`的规范形式是`.`）。
- 路径可以通过`begin() end()`遍历每一个元素，从根名称、根目录到接下来的每一个文件名，其间的目录分隔符被跳过（除了标识根目录的那一个）。如果最后一个元素是一个目录分隔符，那么最后一个迭代器解引用后会是一个空元素。
- 调用所有非const函数都会是引用元素的迭代器失效。
- 在那些目录的路径和文件路径格式不同的系统上，一个通用路径如果末尾有目录分隔符，会被视为目录，否则就是文件。
- 路径可以被隐式转换为`std::basic_string`，以使用其他文件API。
- 分解路径的成员函数返回`std::filesystem::path`而不是字符串。

类型：
- `value_type`：Unix是`char`，Windows上是`wchar_t`，文件系统的字符类型。
- `string_type`：`std::basic_string<value_type>`
- 迭代器：`const_iterator iterator`双向迭代器，`iterator`是`const_iterator`的别名。
- `format`：枚举类型，具有三个值：`native_format generic_format auto_format`，表示本地路径格式、通用路径格式、实现定义的格式（自动检测）。POSIX系统上，本地和通用格式无区别。

常量：
- `constexpr value_type preferred_separator`，目录分隔符，window上是`\`，POSIX系统上是`/`。

构造：
- 默认构造空路径。
- 拷贝、移动构造。
- 从字符串构造，可以是`string_type`，也可以是一个`char char8_t char16_t char32_t wchar_t`字符串或者字符序列（可以以开始结束迭代器标识），后跟一个`format`参数标识格式（默认参数是`auto_format`自动检测）。

赋值：
- 拷贝、移动赋值。
- `operator=`从`string_type`或其他格式的字符串赋值。
- `assign`从`string_type`、其他格式字符或者一对迭代器赋值。

拼接：
- 引入目录分隔符拼接路径：`append /=`。
- 不引入目录分隔符拼接路径：`concat +=`。

修改：
- `clear`：清空。
- `make_preferred`：将所有目录分隔符转换为通用格式，比如window上`foo/bar`会被转换为`foo\bar`。（POSIX中`\`是可以用在文件名中的，这时就不会转换）。
- `remove_filename`：移除文件名。
- `replace_filename`：替换文件名，等价于`remove_filename(); return operator/=(replacement);`
- `replace_extension`：替换扩展名。默认参数为空，此时删除扩展名。
- `swap`：交换两个路径。

格式化观察器：
- `c_str()`, `native()`, `operator string_type()`：将本地路径作为字符串返回。
- 返回各种类型的字符串：`string() wstring() u16string() u32string() u8string()`。
- 返回各种类型的通用格式路径字符串：`generic_string() generic_wstirng() ...`。

比较：
- `compare`：比较当前路径和一个路径、字符串、C风格字符串，字典序。

路径生成：
- `lexically_normal`：生成规范化路径。
- `lexically_relative(base)`：相对某个路径生成相对路径。
- `lexically_proximate(base)`：如果相对路径不为空，返回它，否则返回本身。

分解路径：
- `root_name`: 通用格式的根名称。如果没有根名称，返回空路径。
- `root_directory`: 根目录。
- `root_path`：根路径，等价于`root_name() / root_directory()`。
- `relative_path`: 得到相对于根路径的相对路径。
- `parent_path`: 父路径，如果没有相对路径（`has_reletive_path`返回`false`），那么返回自己。
- `filename`: 文件名，等价于`relative_path().empty() ? path() : *--end()`。
- `stem`: 返回文件名取出扩展名后的名称。
- `extension`：扩展名。

查询：
- `empty`：是否为空。
- `has_root_path has_root_name has_root_directory has_relative_path has_parent_path has_parent_path has_filename has_stem has_extension`：查询是否具有特定路径组成部分（检查特定组成部分是否为空）。
- `is_absolute is_relative`：判断是绝对路径还是相对路径。特别地`/`在POSIX中是一个绝对路径，在windows中是一个相对路径。

迭代器：`begin() end()`。
- 如果在文件名后有一个目录分隔符，那么最后一个元素（文件名后还有一个）是空。如果没有，就没有这个元素。

其他非成员函数：
- `swap`
- `hash_value`
- `operator <=>`比较
- `operator /`拼接两个路径
- `operator << >>`输入输出

帮助类：`std::hash<std::filesystem::path>`模板特化。

## 其他相关类型

- 异常：[`filesystem_error`](https://en.cppreference.com/w/cpp/filesystem/filesystem_error)

- 目录条目：[`directory_entry`](https://en.cppreference.com/w/cpp/filesystem/directory_entry)，保存一个路径，同时保存额外的文件属性（比如硬链接计数、状态、符号链接状态、文件大小、最后写的时间）。

- 目录迭代器：[`directory_iterator`](https://en.cppreference.com/w/cpp/filesystem/directory_iterator)，在目录条目见迭代的迭代器。不会访问子目录中的条目。

- 递归目录迭代器：[recursive_directory_iterator](https://en.cppreference.com/w/cpp/filesystem/recursive_directory_iterator)，递归迭代迭代器，会递归访问子目录中的条目。

- 文件状态：[`file_status`](https://en.cppreference.com/w/cpp/filesystem/file_status)，保存文件类型和权限信息。

- 文件系统中的可用空间信息：[`space_info`](https://en.cppreference.com/w/cpp/filesystem/space_info)。

- 文件类型：
```C++
enum class file_type {
    none = /* unspecified */,
    not_found = /* unspecified */,
    regular = /* unspecified */,
    directory = /* unspecified */,
    symlink = /* unspecified */,
    block = /* unspecified */,
    character = /* unspecified */,
    fifo = /* unspecified */,
    socket = /* unspecified */,
    unknown = /* unspecified */,
    /* implementation-defined */
};
```

- 文件权限信息：
```C++
enum class perms;
```

- 文件权限操作选项：用在文件权限操作函数`std::filesystem::permissions`中。
```C++
enum class perm_options {
    replace = /* unspecified */,
    add = /* unspecified */,
    remove = /* unspecified */,
    nofollow = /* unspecified */
};
```

- 复制选项：用在文件复制函数`std::filesystem::copy std::filesystem::copy_file`中。
```C++
enum class copy_options {
    none = /* unspecified */,
    skip_existing = /* unspecified */,
    overwrite_existing = /* unspecified */,
    update_existing = /* unspecified */,
    recursive = /* unspecified */,
    copy_symlinks = /* unspecified */,
    skip_symlinks = /* unspecified */,
    directories_only = /* unspecified */,
    create_symlinks = /* unspecified */,
    create_hard_links = /* unspecified */
};
```

- 目录操作选项：用在目录条目遍历过程中（通过`directory_iterator` 和`recursive_directory_iterator`）。
```C++
enum class directory_options {
    none = /* unspecified */,
    follow_directory_symlink = /* unspecified */,
    skip_permission_denied = /* unspecified */
};
```

- 文件时间戳类型：`std::filesystem::file_time_type`。

## 文件类型判断

非成员全局函数：
- `std::filesystem::is_block_file`
- `std::filesystem::is_character_file`
- `std::filesystem::is_directory`
- `std::filesystem::is_empty`
- `std::filesystem::is_fifo`
- `std::filesystem::is_other`
- `std::filesystem::is_regular_file`
- `std::filesystem::is_socket`
- `std::filesystem::is_symlink`
- `std::filesystem::status_known`

## 文件操作函数

- 获取绝对路径：`absolute`
- 获取规范路径：`canonical weakly_canonical`
- 获取相对路径：`relative proximate`
- 复制文件或目录：`copy`
- 复制文件内容：`copy_file`
- 复制符号链接：`copy_symlink`
- 创建新目录：`create_directory create_directories`
- 创建硬链接：`create_hard_link`
- 创建符号链接：`create_symlink create_directory_symlink`
- 当前工作目录：`current_path`
- 检查路径在文件系统中是否存在：`exists`
- 检查两个路径是否等价：`equivalent`
- 文件尺寸：`file_size`
- 硬链接数量：`hard_link_count`
- 最后写入时间：`last_write_time`
- 修改文件权限：`permissions`
- 读取符号链接的目标：`read_symlink`
- 移除文件或者空目录：`remove`
- 移除文件或者递归移除目录所有内容：`remove_all`
- 移动或者重命名文件：`rename`
- 修改大小（通过截断或者填充0）：`resize_file`
- 确定文件系统中某个路径的位置的剩余空间：`space`
- 查看文件状态：`status`
- 查看符号链接的文件的状态（即不跟随符号链接）：`symlink_status`
- 返回合适的临时文件的目录：`temp_directory_path`
