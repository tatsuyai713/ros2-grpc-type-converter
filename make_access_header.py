#!/usr/bin/env python3

import os
import sys
import re
from pathlib import Path

def parse_pb_header(pb_header_path):
    """
    .pb.h ファイルを解析し、メッセージフィールドの情報を抽出する。
    不要なフィールド (例: Metadata) を除外し、標準型の場合は is_message を False に設定。
    """
    if not os.path.exists(pb_header_path):
        print(f"Warning: {pb_header_path} does not exist.")
        return {}

    with open(pb_header_path, 'r', encoding='utf-8') as f:
        content = f.read()

    fields = {}

    # スコープの状態を追跡する
    current_scope = None

    # 除外対象のフィールド名 (必要に応じて追加)
    exclude_fields = ["metadata", "GetMetadata"]

    # 標準型のリスト
    simple_types = [
        'int32', 'int64', 'uint32', 'uint64', 'float', 'double',
        'bool', 'string', 'std::string', 'int32_t', 'int64_t',
        'uint32_t', 'uint64_t'
    ]

    # 全体を行単位で解析
    for line in content.splitlines():
        line = line.strip()

        # スコープの変更を確認
        if line.startswith("public:"):
            current_scope = "public"
            continue
        elif line.startswith("private:"):
            current_scope = "private"
            continue

        # スコープが public でない場合、無視
        if current_scope != "public":
            continue


        # Getter: const <type>& <field>() const;
        getter_pattern = re.compile(
            r'const\s+([^\s]+(?:\s*::\s*[^\s]+)*)&\s+(\w+)\s*\(\)\s*const\s*;'
        )
        # Setter: void set_<field>(const <type>& value);
        setter_pattern = re.compile(
            r'void\s+set_(\w+)\s*\(\s*(?:const\s+)?([^\s]+(?:\s*::\s*[^\s]+)*)\s*(?:&)?\s+value\s*\)\s*;'
        )
        # Mutable: <type>* mutable_<field>();
        mutable_pattern = re.compile(
            r'([^\s]+(?:\s*::\s*[^\s]+)*)\*\s+mutable_(\w+)\s*\(\)\s*;'
        )
        # add_<field>(): for repeated fields
        add_pattern = re.compile(
            r'([^\s]+(?:\s*::\s*[^\s]+)*)\*\s+add_(\w+)\s*\(\)\s*;'
        )
        # size_<field>(): for repeated fields
        size_pattern = re.compile(
            r'int\s+(\w+)_size\(\)\s*const\s*;'
        )
        # set_allocated_<field>(<type>* value);
        set_allocated_pattern = re.compile(
            r'void\s+set_allocated_(\w+)\s*\(\s*([^\s]+(?:\s*::\s*[^\s]+)*)\*\s+value\s*\)\s*;'
        )
        
        # Getter の解析
        getter_match = getter_pattern.match(line)
        if getter_match:
            field_type, field_name = getter_match.groups()
            if field_name.startswith("allocated_"):
                continue
            # 除外対象のフィールドをスキップ
            if field_name.lower() in (name.lower() for name in exclude_fields):
                continue
            fields.setdefault(field_name, {})
            fields[field_name]['getter'] = {
                'return_type': field_type.strip(),
            }
            continue

        # Setter の解析
        setter_match = setter_pattern.match(line)
        if setter_match:
            field_name, field_type = setter_match.groups()
            if field_name.startswith("allocated_"):
                continue
            # 除外対象のフィールドをスキップ
            if field_name.lower() in (name.lower() for name in exclude_fields):
                continue
            fields.setdefault(field_name, {})
            fields[field_name]['setter'] = {
                'param_type': field_type.strip(),
            }
            continue

        # Mutable の解析
        mutable_match = mutable_pattern.match(line)
        if mutable_match:
            field_type, field_name = mutable_match.groups()
            if field_name.startswith("allocated_"):
                continue
            # 除外対象のフィールドをスキップ
            if field_name.lower() in (name.lower() for name in exclude_fields):
                continue
            fields.setdefault(field_name, {})
            fields[field_name]['mutable'] = {
                'return_type': field_type.strip(),
            }
            continue

        # add_<field> の抽出
        add_match = add_pattern.match(line)
        if add_match:
            field_type, field_name = add_match.groups()
            if field_name.startswith("allocated_"):
                continue
            fields.setdefault(field_name, {})
            fields[field_name]['add'] = {
                'return_type': field_type.strip(),
            }
            continue

        # size_<field> の抽出
        size_match = size_pattern.match(line)
        if size_match:
            field_name, = size_match.groups()
            if field_name.startswith("allocated_"):
                continue
            fields.setdefault(field_name, {})
            fields[field_name]['size'] = {}
            continue

        # set_allocated_<field> の抽出
        set_allocated_match = set_allocated_pattern.match(line)
        if set_allocated_match:
            field_name, field_type = set_allocated_match.groups()
            if field_name.startswith("allocated_"):
                continue
            fields.setdefault(field_name, {})
            fields[field_name]['set_allocated'] = {
                'param_type': field_type.strip(),
            }
            continue
        
    # repeated 判定とメッセージ型の判定
    for field_name, info in fields.items():
        
        info['is_message'] = False

    # メッセージ型かどうかの判定
    simple_types = ['int32_t', 'int64_t', 'uint32_t', 'uint64_t', 'float', 'double', 'bool', 'string', 'std::string']
    
    for field_name, info in fields.items():
        base_type = ''
        if 'getter' in info:
            base_type = info['getter']['return_type'].replace('&', '').replace('*', '').strip()
        elif 'setter' in info:
            base_type = info['setter']['param_type'].replace('&', '').replace('*', '').strip()
        elif 'mutable' in info:
            base_type = info['mutable']['return_type'].replace('&', '').replace('*', '').strip()
        elif 'add' in info:
            base_type = info['add']['return_type'].replace('&', '').replace('*', '').strip()
        elif 'set_allocated' in info:
            base_type = info['set_allocated']['param_type'].replace('&', '').replace('*', '').strip()
            
        base_type_no_ns = base_type.split('::')[-1]
        if base_type_no_ns in simple_types:
            info['is_message'] = False  # 標準型は False
        else:
            info['is_message'] = True   # 標準型以外はメッセージ型

    return fields


def find_corresponding_files(idl_file):
    """
    指定された .idl ファイルに対応する .pb.h および .grpc.pb.h ファイルを見つける。
    （同じディレクトリで名前が一致する .pb.h と .grpc.pb.h を探す簡単な実装）
    """
    base_name = os.path.splitext(os.path.basename(idl_file))[0]
    dir_path = os.path.dirname(idl_file)

    pb_h = os.path.join(dir_path, f"{base_name}.pb.h")
    grpc_pb_h = os.path.join(dir_path, f"{base_name}.grpc.pb.h")

    return pb_h, grpc_pb_h


def convert_grpc_type_to_include_path(grpc_type: str) -> str:
    """
    例:
      - "std_msgs::msg::HeaderGRPC" -> "std_msgs/msg/header.hpp"
      - "::geometry_msgs::msg::TransformStampedGRPC" -> "geometry_msgs/msg/transformstamped.hpp"
      - "google::protobuf::RepeatedPtrField<geometry_msgs::msg::TransformStampedGRPC>"
         -> "geometry_msgs/msg/transformstamped.hpp"
      - "std::string" -> "" (do not include)
      - "google::protobuf::RepeatedPtrField<std::string>" -> "" (do not include)

    処理フロー:
      1) もし "<...>" を含むなら、その中だけ取り出して再帰呼び出し
      2) 末尾 "GRPC" を取り除く
      3) '::' の数を数える
         3.1) 2つ以上ならメッセージ型として処理
         3.2) それ以外なら、標準型としてスキップ
      4) "::" -> "/"
      5) 先頭スラッシュがあれば削除
      6) 最後の要素を小文字化
      7) ".hpp" を付ける
    """

    # 1) もし "<...>" を含むなら、その中身だけ取り出して再帰呼び出し
    bracket_pattern = r'^(.*)<([^>]+)>(.*)$'
    m = re.match(bracket_pattern, grpc_type.strip())
    if m:
        inside = m.group(2).strip()
        return convert_grpc_type_to_include_path(inside)

    # 2) 末尾 "GRPC" を除去
    if grpc_type.endswith("GRPC"):
        grpc_type = grpc_type[:-4]

    # 3) '::' の数を数える
    num_colons = grpc_type.count("::")
    if num_colons < 2:
        # Not a message type, skip
        return ""

    # 4) "::" -> "/"
    path = grpc_type.replace("::", "/")

    # 5) 先頭スラッシュがあれば削除
    if path.startswith("/"):
        path = path[1:]

    # 6) 最後の要素を小文字化
    parts = path.split('/')
    parts[-1] = parts[-1].lower()
    path = "/".join(parts)

    # 7) ".hpp" を付与
    path += ".hpp"

    return path


def generate_wrapper_class(directory, namespace, message_name, fields):
    """
    指定されたメッセージ名とフィールド情報に基づき、C++ラッパークラスを生成する。
    FastDDS風に、 repeated フィールドのうち numeric なものは RepeatedFieldWrapper<T> で扱う。
    """
    class_name = message_name  # Wrapper class name
    grpc_class_name = f"{message_name}GRPC"
    grpc_full_class = f"{grpc_class_name}" if not namespace else f"{namespace}::{grpc_class_name}"

    header_filename = f"{class_name.lower()}.hpp"
    header_path = os.path.join(directory, header_filename)

    # インクルードリスト
    modified_grpc_class_name = grpc_class_name.replace("GRPC", "")
    includes = [
        '#pragma once',
        '#include <memory>',
        '#include <string>',
        '#include <vector>',
        '#include <grpcpp/grpcpp.h>',
        '#include "google/protobuf/empty.pb.h"',
        '#include "google/protobuf/repeated_field.h"',
        f'#include "{modified_grpc_class_name}.pb.h"',
        f'#include "{modified_grpc_class_name}.grpc.pb.h"',
    ]

    # サブメッセージのインクルードパスを収集
    sub_includes = set()
    for field_name, info in fields.items():
        if info.get('is_message'):
            grpc_type = ''
            if 'getter' in info:
                grpc_type = info['getter']['return_type']
            elif 'setter' in info:
                grpc_type = info['setter']['param_type']
            elif 'mutable' in info:
                grpc_type = info['mutable']['return_type']
            elif 'add' in info:
                grpc_type = info['add']['return_type']
            elif 'set_allocated' in info:
                grpc_type = info['set_allocated']['param_type']
            inc_path = convert_grpc_type_to_include_path(grpc_type)
            if inc_path:
                if not "*" in inc_path:
                    sub_includes.add(inc_path)

    # サブメッセージのインクルードを追加
    for inc in sorted(sub_includes):
        includes.append(f'#include "{inc}"  // 2階層上フォルダからの相対指定を想定')

    member_vars = []
    initializer_list = []
    sendgrpc_sync_to_grpc = []
    accessor_methods = []
    delete_members = []
    repeated_field_wrapper_code = ""
    proxy_class = ""

    for field_name, info in fields.items():
        cpp_type = (
            info.get('getter', {}).get('return_type') or
            info.get('setter', {}).get('param_type') or
            info.get('mutable', {}).get('return_type') or
            'void'
        ).strip()
        is_message = info['is_message']

        if is_message:
            underlying_type = cpp_type.replace('GRPC', '').strip()
            if("RepeatedField" in underlying_type):
                # ラッパークラス内で使う detail 名前空間の定義 (RepeatedFieldWrapper)
                repeated_field_wrapper_code = """
#ifndef REPEATED_FIELD_WRAPPER_HPP
#define REPEATED_FIELD_WRAPPER_HPP
namespace detail {

template <typename T>
class RepeatedFieldWrapper {
public:
    explicit RepeatedFieldWrapper(::google::protobuf::RepeatedField<T>* field) : field_(field) {}

    // operator[]
    T& operator[](size_t index) { 
        if (!field_) {
            throw std::runtime_error("RepeatedPtrField is not initialized");
        }
        return *field_->Mutable(index); 
    }
    const T& operator[](size_t index) const { 
        if (!field_) {
            throw std::runtime_error("RepeatedPtrField is not initialized");
        }
        return field_->Get(index); 
    }

    // push_back
    void push_back(const T& value) { field_->Add(value); }

    // erase
    void erase(size_t index) {
        if (index >= size()) throw std::out_of_range("Index out of range");
        field_->DeleteSubrange(index, 1);
    }

    // size, empty
    size_t size() const { return field_->size(); }
    bool empty() const { return field_->empty(); }
    
    // resize
    void resize(size_t new_size) { field_->Resize(new_size, T()); }

    // clear
    void clear() { field_->Clear(); }

private:
    ::google::protobuf::RepeatedField<T>* field_;
};

} // namespace detail
#endif // REPEATED_FIELD_WRAPPER_HPP
"""

                # numeric な repeated => RepeatedFieldWrapper<T>
                # 例) repeated uint32_t -> detail::RepeatedFieldWrapper<uint32_t>
                type = cpp_type.split('<')[-1].split('>')[0].strip()
                wrapper_type = f"detail::RepeatedFieldWrapper<{type}>"
                member_vars.append(f"    {wrapper_type} {field_name}_;")
                # コンストラクタの初期化子
                initializer_list.append(f"{field_name}_(grpc_->mutable_{field_name}())")
                # アクセッサ
                accessor_methods.append(
                    f"    {wrapper_type}& {field_name}() {{ return {field_name}_; }}\n"
                    f"    const {wrapper_type}& {field_name}() const {{ return {field_name}_; }}"
                )
                # send() 時の同期は不要 (直接 RepeatedField を操作しているため)
            
            elif("RepeatedPtrField" in underlying_type):
                # ラッパークラス内で使う detail 名前空間の定義 (RepeatedFieldWrapper)
                repeated_field_wrapper_code = f"""\
#ifndef REPEATED_PTR_FIELD_WRAPPER_HPP
#define REPEATED_PTR_FIELD_WRAPPER_HPP
namespace detail {{
template <typename T, typename S>
class RepeatedPtrFieldWrapper {{
public:
    explicit RepeatedPtrFieldWrapper(::google::protobuf::RepeatedPtrField<S>* field) : field_(field) {{
        for (int i = 0; i < field_->size(); ++i) {{
            elements_.push_back(new T(field_->Mutable(i)));
        }}
    }}

    // operator[]
    T& operator[](size_t index) {{
        if (!field_) {{
            throw std::runtime_error("RepeatedPtrField is not initialized");
        }}
        return *elements_[index];
    }}
    const T& operator[](size_t index) const {{
        if (!field_) {{
            throw std::runtime_error("RepeatedPtrField is not initialized");
        }}
        return *elements_[index];
    }}

    // push_back
    void push_back(const T& value) {{
        field_->Add(value.get_grpc());
        elements_.push_back(new T(field_->Mutable(field_->size() - 1)));
    }}

    // erase
    void erase(size_t index) {{
        if (index >= size()) throw std::out_of_range("Index out of range");
        field_->DeleteSubrange(index, 1);
        delete elements_[index];
        elements_.erase(elements_.begin() + index);
    }}

    // size, empty
    size_t size() const {{ return field_->size(); }}
    bool empty() const {{ 
        for (auto& elem : elements_) {{
            delete elem;
        }}
        elements_.clear();
        return field_->empty(); 
    }}
    
    // resize
    void resize(size_t new_size) {{
        if (new_size > field_->size()) {{
            // 要素数を増やす
            for (size_t i = field_->size(); i < new_size; ++i) {{
                auto* new_element = field_->Add();
                elements_.push_back(new T(new_element));
            }}
        }} else if (new_size < field_->size()) {{
            // 要素数を減らす
            for (size_t i = field_->size() - 1; i >= new_size; --i) {{
                delete elements_[i];
            }}
            field_->DeleteSubrange(new_size, field_->size() - new_size);
            elements_.resize(new_size);
        }}
    }}

    // clear
    void clear() {{
        field_->Clear();
        elements_.clear();
    }}

private:
    ::google::protobuf::RepeatedPtrField<S>* field_;
    std::vector<T*> elements_;
}};

}} // namespace detail
#endif // REPEATED_PTR_FIELD_WRAPPER_HPP
"""
                # numeric な repeated => RepeatedPtrFieldWrapper<T>
                type = cpp_type.split('<')[-1].split('>')[0].strip()
                type_without_grpc = type.replace("GRPC", "")
                wrapper_type = f"detail::RepeatedPtrFieldWrapper<{type_without_grpc}, {type}>"
                member_vars.append(f"    {wrapper_type} {field_name}_;")
                # コンストラクタの初期化子
                initializer_list.append(f"{field_name}_({wrapper_type}(grpc_->mutable_{field_name}()))")
                # アクセッサ
                accessor_methods.append(
                    f"    {wrapper_type}& {field_name}() {{ return {field_name}_; }}\n"
                    f"    const {wrapper_type}& {field_name}() const {{ return {field_name}_; }}"
                )
                # send() 時の同期は不要 (直接 RepeatedPtrFieldWrapper を操作しているため)
            
            else:
                member_vars.append(f"    {underlying_type}* {field_name}_;")
                # アクセッサメソッドを実際の型の参照を返すように
                accessor_methods.append(
                    f"    {underlying_type}& {field_name}() {{ return *{field_name}_; }}\n"
                    f"    const {underlying_type}& {field_name}() const {{ return *{field_name}_; }}"
                )
                # 初期化リストに追加
                initializer_list.append(
                    f"{field_name}_(new {underlying_type}(grpc_->mutable_{field_name}()))"
                )
                # デストラクタで削除
                delete_members.append(f"        delete {field_name}_;")
        else:
            if cpp_type in ['std::string', 'std::string&']:
                # std::string の場合
                accessor_methods.append(
                    f"    std::string& {field_name}() {{ return *grpc_->mutable_{field_name}(); }}\n"
                    f"    const std::string& {field_name}() const {{ return grpc_->{field_name}(); }}\n"
                    f"    void {field_name}(const std::string& value) {{ grpc_->set_{field_name}(value); }}"
                )
            else:
                # 標準型フィールド
                accessor_methods.append(f"""
    {class_name}{field_name}Proxy {field_name}() {{
        return {class_name}{field_name}Proxy(grpc_);
    }}
    void {field_name}({cpp_type} value) {{
        grpc_->set_{field_name}(value);
    }}
    """
                )
                # Proxy クラスを生成
                proxy_class += f"""
class {class_name}{field_name}Proxy {{
public:
    {class_name}{field_name}Proxy({grpc_full_class}* grpc)
        : grpc_(grpc) {{}}

    {class_name}{field_name}Proxy& operator=({cpp_type} value) {{
        grpc_->set_{field_name}(value);
        return *this;
    }}
    
    operator {cpp_type}() const {{
        return grpc_->{field_name}();
    }}
    
private:
    {grpc_full_class}* grpc_;
}};
"""


    # namespace
    if namespace:
        namespace_declaration = f"namespace {namespace} {{"
        namespace_closing = f"}} // namespace {namespace}"
    else:
        namespace_declaration = ""
        namespace_closing = ""

    # コンストラクタ
    constructor_initializers = [f"grpc_(new {grpc_full_class}()), data_owned_(true)"]
    constructor_initializers += initializer_list

    constructor_code = ""
    if initializer_list:
        constructor_code = (
            f"    {class_name}()\n"
            "        : " + ",\n          ".join(constructor_initializers) + "\n"
            "    {\n" +
            "\n    }"
        )
    else:
        constructor_code = (
            f"    {class_name}()\n"
            f"        : grpc_(new {grpc_full_class}())\n"
            f"    {{\n    }}"
        )

    # If you need to handle other constructors, such as wrapping an existing GRPC pointer
    explicit_constructor = (
        f"    explicit {class_name}({grpc_full_class}* grpc_ptr)\n"
        f"        : grpc_(grpc_ptr), data_owned_(false)\n"
    )
    # Initialize message-type member variables in the explicit constructor
    explicit_initializers = []
    explicit_initializers += initializer_list
    if explicit_initializers:
        explicit_constructor += (
            "        , " + ",\n          ".join(explicit_initializers) + "\n"
            "    {\n"
            "    }"
        )
    else:
        explicit_constructor += "    {\n    }"

    # sendgrpc_code
    sendgrpc_code = ""
    if sendgrpc_sync_to_grpc:
        sendgrpc_code += (
            "        // Sync from local members to grpc_\n" +
            "\n".join(sendgrpc_sync_to_grpc)
        )

    # members_def
    members_def = "\n".join(member_vars)

    # accessors_def
    accessors_def = "\n\n".join(accessor_methods)

    # コピーコンストラクタとコピー代入演算子の生成
    copy_constructor = f"""\
    // コピーコンストラクタ
    {class_name}(const {class_name}& other)
        : grpc_(other.grpc_), data_owned_(false)"""
    if member_vars:
        copy_initializers = []
        for field_name, info in fields.items():
            if info['is_message']:
                copy_initializers.append(f"{field_name}_(other.{field_name}_)")
        if copy_initializers:
            copy_constructor += ",\n          " + ",\n          ".join(copy_initializers)
    copy_constructor += "\n    {}"

    # デストラクタ
    destructor = f"""\
    // デストラクタ
    ~{class_name}() {{
        if (data_owned_) {{
            delete grpc_;
        }}
{os.linesep.join(delete_members)}
    }}"""
    
    # ()オペレータ
    copy_operator = f"""\
    void operator()(const {class_name}& other)"""
    copy_operator += "\n    {\n        grpc_->CopyFrom(*other.grpc_);\n    }"
    
    # =オペレータ
    equal_operator = f"""\
    void operator=(const {class_name}& other)"""
    equal_operator += "\n    {\n        grpc_->CopyFrom(*other.grpc_);\n    }"
    
    # Get grpc_ pointer
    get_accessor = f"    {grpc_full_class}* get_grpc() {{ return grpc_; }}"
    
    # Type definition for subscription
    type_def = f"    {grpc_full_class} type_;"

    # クラス定義
    header_content = f"""\
{os.linesep.join(includes)}
{repeated_field_wrapper_code}
{namespace_declaration}
{proxy_class}
class {class_name} : public {grpc_full_class}Service::Service {{
public:
{constructor_code}
{explicit_constructor}
{copy_constructor}
{destructor}
{copy_operator}
{equal_operator}
{get_accessor}
{type_def}
    // ========== アクセサメソッド群 ==========
{accessors_def}
    // gRPC サービスの Stub を初期化
    void NewStub(std::shared_ptr<grpc::Channel> channel) {{
        stub_ = {grpc_full_class}Service::NewStub(channel);
    }}
    // RPC メソッドの呼び出し
    grpc::Status send(grpc::ClientContext& context) {{
{sendgrpc_code}
        google::protobuf::Empty empty;
        if (!stub_) {{
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Stub not initialized");
        }}
        return stub_->SendGRPC(&context, *grpc_, &empty);
    }}
private:
    {grpc_full_class}* grpc_;
    bool data_owned_{{false}};
{members_def}
    std::shared_ptr<{grpc_full_class}Service::Stub> stub_;
}};

{namespace_closing}
"""

    # 出力ファイルへ書き込み
    with open(header_path, 'w', encoding='utf-8') as f:
        f.write(header_content)

    print(f"Generated wrapper: {header_path}")


def process_idl_file(idl_file):
    """
    個別の .idl ファイルを処理し、対応するラッパークラスを生成する。
    生成されたヘッダーファイルは、.pb.h ファイルと同じディレクトリに配置される。
    """
    pb_h, grpc_pb_h = find_corresponding_files(idl_file)
    if not os.path.exists(pb_h) or not os.path.exists(grpc_pb_h):
        print(f"Warning: Corresponding .pb.h or .grpc.pb.h for {idl_file} not found. Skipping.")
        return

    # メッセージ名は、.idlのファイル名ベース
    message_name = os.path.splitext(os.path.basename(idl_file))[0]

    # 名前空間をディレクトリ構造から推測
    relative_path = os.path.relpath(idl_file, start=base_dir)
    path_parts = Path(relative_path).parents
    namespace_parts = []
    for part in reversed(path_parts):
        if part == Path('.'):
            continue
        namespace_parts.append(part.name)
    namespace = '::'.join(namespace_parts) if namespace_parts else ''

    # フィールド情報を抽出
    fields = parse_pb_header(pb_h)
    # ラッパークラスの生成
    directory = os.path.dirname(pb_h)
    generate_wrapper_class(directory, namespace, message_name, fields)


def search_idl_files(base_dir):
    """
    base_dir 以下を再帰的に探索し、.idl ファイルをすべて収集
    """
    idl_files = []
    for root, dirs, files in os.walk(base_dir):
        for file in files:
            if file.endswith('.idl'):
                idl_file_path = os.path.join(root, file)
                idl_files.append(idl_file_path)
    return idl_files


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 make_access_header.py <base_directory>")
        sys.exit(1)

    global base_dir
    base_dir = os.path.abspath(sys.argv[1])
    if not os.path.isdir(base_dir):
        print(f"Error: {base_dir} is not a valid directory.")
        sys.exit(1)

    # 再帰的にIDLファイルを探す
    idl_files = search_idl_files(base_dir)
    if not idl_files:
        print(f"No .idl files found in {base_dir}.")
        sys.exit(0)

    # それぞれのIDLを処理
    for idl_file in idl_files:
        print(f"Processing: {idl_file}")
        process_idl_file(idl_file)


if __name__ == "__main__":
    main()
