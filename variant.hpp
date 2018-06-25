// ///////////////////////////////////////////////////////////////////////// //
//                                                                           //
//   Copyright (C) 2018 by Oleg Polivets                                     //
//   jsbot@ya.ru                                                             //
//                                                                           //
//   This program is free software; you can redistribute it and/or modify    //
//   it under the terms of the GNU General Public License as published by    //
//   the Free Software Foundation; either version 2 of the License, or       //
//   (at your option) any later version.                                     //
//                                                                           //
//   This program is distributed in the hope that it will be useful,         //
//   but WITHOUT ANY WARRANTY; without even the implied warranty of          //
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           //
//   GNU General Public License for more details.                            //
//                                                                           //
// ///////////////////////////////////////////////////////////////////////// //

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iomanip>
#include <cassert>

namespace op {

class Variant;
typedef std::shared_ptr<Variant> VariantPtr;
typedef std::vector<VariantPtr> ValuesVector;
typedef std::map<std::string, VariantPtr> KeyValueMap;

class Variant {
public:
    enum TYPE {
        vtEmpty,
        vtString,
        vtInteger,
        vtFloat,
        vtDouble,
        vtRepeated,
        vtNode
    };

    explicit Variant(TYPE dataType = vtEmpty) : mDataType(dataType)
    {}

    explicit Variant(const char* string, unsigned lenght)
        : mDataType(vtString)
        , mString(string, lenght)
    {}

    explicit Variant(int64_t value) : mDataType(vtInteger) {
        mData.i = value;
    }

    explicit Variant(double value) : mDataType(vtDouble) {
        mData.d = value;
    }

    explicit Variant(float value) : mDataType(vtFloat) {
        mData.f = value;
    }

    ~Variant() {}

    template <class T>
    bool hasField(const T & index) const {
        assert(isMap());
        KeyValueMap::const_iterator it = mNodes.find(index);
        return (it != mNodes.end());
    }

    template <class T>
    VariantPtr operator[] (const T & idx) {
        if (isMap()) {
            return mNodes[idx];
        } else if (isRepeated()) {
            return mItems[idx];
        }
        assert(!"this shouldn't happen");
        return nullptr;
    }

    static VariantPtr make() {
        return VariantPtr(new Variant());
    }
    static VariantPtr makeMap() {
        return VariantPtr(new Variant(vtNode));
    }
    static VariantPtr makeRepeated() {
        return VariantPtr(new Variant(vtRepeated));
    }
    static VariantPtr make(const char * data, unsigned lenght) {
        return VariantPtr(new Variant(data, lenght));
    }
    template <class T>
    static VariantPtr make(T value) {
        return VariantPtr(new Variant(value));
    }

    bool isRepeated() const {
        return mDataType == vtRepeated;
    }
    ValuesVector & asVector() {
        assert(isRepeated());
        return mItems;
    }
    const ValuesVector & asVector() const {
        assert(isRepeated());
        return mItems;
    }

    bool isString() const {
        return mDataType == vtString;
    }
    const std::string & asString() const {
        assert(isString());
        return mString;
    }
    std::string & asString() {
//        assert(isString());
        return mString;
    }

    bool isInt() const {
        return mDataType == vtInteger;
    }
    int64_t asInt() const {
        assert(isInt());
        return mData.i;
    }
    int64_t& asInt() {
        assert(isInt());
        return mData.i;
    }

    bool isFloat() const {
        return mDataType == vtFloat;
    }
    float asFloat() const {
        assert(isFloat());
        return mData.f;
    }
    float& asFloat() {
        assert(isFloat());
        return mData.f;
    }

    bool isDouble() const {
        return mDataType == vtDouble;
    }
    double asDouble() const {
        assert(isDouble());
        return mData.d;
    }
    double& asDouble() {
        assert(isDouble());
        return mData.d;
    }

    bool isMap() const {
        return (mDataType == vtNode);
    }
    const KeyValueMap & asMap() const {
        assert(isMap());
        return mNodes;
    }
    KeyValueMap & asMap() {
        assert(isMap());
        return mNodes;
    }
    template <class T>
    const std::string & asStringMap(const T & idx) const {
        assert(isMap());
        KeyValueMap::const_iterator it = mNodes.find(idx);
        assert(it != mNodes.end());
        return it->second->asString();
    }
    template <class T>
    std::string & asStringMap(const T & idx) {
        assert(isMap());
        return mNodes[idx]->asString();
    }

    const char* dataType() const {
        switch (mDataType) {
        case vtInteger:  return "int64";
        case vtDouble:   return "double";
        case vtString:   return "string";
        case vtFloat:    return "float";
        case vtRepeated: return "vector";
        case vtNode:     return "map";
        default: {
            assert(!"This shouldn't happen.");
            return nullptr;
        }}
    }

    friend std::ostream & operator<<(std::ostream & os, const Variant & var);

    static void printMapInternal(
        const KeyValueMap & map,
        std::ostream & os,
        int indent = 0
    ) {
        size_t i = 0, ie = map.size()-1;
        for (KeyValueMap::const_iterator it = map.begin(); it != map.end(); ++it, ++i) {
            const VariantPtr & var = it->second;
            if (var->isMap()) {
                KeyValueMap & map = var->asMap();
                //assert(!map.empty());
                for (int i = 0; i < indent; ++i) os << '\t';
                os << "\"" << it->first << "\":" << " {\n";
                printMapInternal(map, os, indent+1);
                for (int i = 0; i < indent; ++i) os << '\t';
                os << "}";
            } else if (var->isRepeated()) {
                ValuesVector & vec = var->asVector();
                //assert(!vec.empty());
                for (int i = 0; i < indent; ++i) os << '\t';
                os << "\"" << it->first << "\"" << ":" << " [\n";
                printVecInternal(vec, os, indent+1);
                for (int i = 0; i < indent; ++i) os << '\t';
                os << "]";
            } else {
                for (int i = 0; i < indent; ++i) os << '\t';
                os << "\"" << it->first << "\"" << ": " << *var;
            }
            if (i != ie) os << ',';
            os << '\n';
        }
    }

    static void printVecInternal(
        const ValuesVector & vec,
        std::ostream & os,
        int indent = 0
    ) {
        for (unsigned i = 0; i < vec.size(); ++i) {
            const VariantPtr & var = vec[i];
            if (var->isMap()) {
                KeyValueMap & map = var->asMap();
                assert(!map.empty());
                for (int i = 0; i < indent; ++i) os << '\t';
                os << "{\n";
                printMapInternal(map, os, indent+1);
                for (int i = 0; i < indent; ++i) os << '\t';
                os << "}";
            } else if (var->isRepeated()) {
                ValuesVector & vec = var->asVector();
                assert(!vec.empty());
                for (int i = 0; i < indent; ++i) os << '\t';
                os << "[\n";
                printVecInternal(vec, os, indent+1);
                for (int i = 0; i < indent; ++i) os << '\t';
                os << "]";
            } else {
                for (int i = 0; i < indent; ++i) os << '\t';
                os << *var;
            }
            if (i != vec.size()-1) os << ',';
            os << '\n';
        }
    }

    void print(std::ostream & os, int indent = 0) const {
        if (this->isMap()) {
            os << "[\n";
            printMapInternal(this->asMap(), os, indent);
            os << "}\n";
        } else if (this->isRepeated()) {
            os << "[\n";
            printVecInternal(this->asVector(), os, indent);
            os << "]\n";
        } else {
            for (int i = 0; i < indent; ++i) os << '\t';
            os << *this << '\n';
        }
    }

    // /////////////////////////////////////////////////////////////////// //

private:
    TYPE mDataType;
    union {              // wrap access to int, float and double data types
        int64_t i;
        float f;
        double d;
    } mData;
    std::string mString; // data for vtString and content of message for vtMap
    KeyValueMap mNodes;  // subnodes vtMap data type
    ValuesVector mItems; // for vtRepeated
}; // Variant

// /////////////////////////////////////////////////////////////////// //

inline std::ostream & operator<<(std::ostream & os, const Variant & var) {
    if (var.isInt()) {
        os /*<< "int64:"*/ << std::dec << var.asInt();
    } else if (var.isFloat()) {
        os /*<< "float:"*/ << var.asFloat();
    } else if (var.isDouble()) {
        os /*<< "double:"*/ << var.asDouble();
    } else if (var.isString()) {
        os << '"';
        const std::string & str = var.asString();
        for (unsigned i = 0; i < str.length(); ++i) {
            bool outAsHex = ((str[i] >=0x00 && str[i] <= 0x1F) /* || ch > 0x7f */);
            if (!outAsHex) {
                if (str[i] == '"' || str[i] == '\\' /* || str[i] == '/' */)
                    os << '\\';
                os << str[i];
            } else {
                switch (str[i]) {
                case '\n': os << "\\n"; break;
                case '\r': os << "\\r"; break;
                case '\t': os << "\\t"; break;
                case '\b': os << "\\b"; break;
                case '\f': os << "\\f"; break;
                default:
                    os << "\\u" << std::setfill('0') << std::setw(4) << std::hex << (int) str[i];
                    break;
                }
            }
        }
        os << '"';
    } else {
        assert(!"This shouldn't happen.");
    }
    return os;
}

} // namespace op
