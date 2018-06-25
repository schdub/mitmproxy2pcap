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

#include "variant.hpp"
#include <sstream>
#include <stdexcept>

namespace op {

/*
 * Class with example how to parse netstrings serialization method.
 */

class MFlowParser {
public:

    template <class T>
    static const VariantPtr At(const KeyValueMap & map, const T & k) {
        KeyValueMap::const_iterator it = map.find(k);
        if (it != map.end()) {
            return it->second;
        }
        std::stringstream ss;
        ss << "ERROR: Key '" << k << "' not found.";
        throw std::logic_error(ss.str());
    }

    VariantPtr rootItem() const {
        assert(mRoot);
        return mRoot;
    }
    KeyValueMap & itemsMap() const {
        assert(mRoot && !mRoot->asMap().empty());
        return mRoot->asMap();
    }
    ValuesVector & itemsVec() const {
        return mRoot->asVector();
    }

    template <class T>
    const VariantPtr operator[] (const T & idx) const {
        assert(!mRoot->asMap().empty());
        KeyValueMap::const_iterator it = mRoot->asMap().find(idx);
        assert(it != mRoot->asMap().end());
        return it->second;
    }

    // /////////////////////////////////////////////////////////////////// //

    char popStr(const char *& pBegin, const char * pEnd, std::string & buffer) {
        buffer.clear();
        char slen[10];
        size_t len = 0;
        // read data length
        len = 0;
        while (len < sizeof(slen)-1) {
            if (pBegin >= pEnd) return 'E';
            slen[len] = *pBegin++;
            if (!::isdigit(slen[len])) break;
            ++len;
        }
        slen[len] = '\0';

        // nothing?
        if (len == 0) return 'E';

        len = atoi(slen);

//        std::cerr << "LEN = " << len << std::endl;

        // read data
        buffer = std::string(pBegin, len);
        pBegin += len;
        assert (pBegin < pEnd);

//        std::cerr << "DATA = " << buffer << std::endl;

        // read data type
        *slen = '\0';
        *slen = *pBegin++;
        assert (pBegin <= pEnd);

//        std::cerr << "DTYPE = " << *slen << std::endl;

        return *slen;
    }

    /*
     *
     */

    const char *  parseVector(const char * pBegin, const char * pEnd, ValuesVector & vec) {
//        std::cerr << "PARSE VEC" << std::endl;
        std::string buffer;
        while (pBegin < pEnd) {
            char dataType = popStr(pBegin, pEnd, buffer);

            switch (dataType) {
            case ',':
            case ';':
            case '!':
            case '#':
            case '^':
            case '~':
                vec.push_back(Variant::make(buffer.c_str(), buffer.length()));
                break;
#if (0)
            // TODO: enable parsing of this data types
            case '!':
                vec.push_back(Variant::make((int64_t) !buffer.compare("true")));
                break;
            case '#':
                vec.push_back(Variant::make(atoi(buffer.c_str())));
                break;
            case '^':
                vec.push_back(Variant::make(atof(buffer.c_str())));
                break;
#endif
            case '}': {
                VariantPtr newMap = Variant::makeMap();
                parseMap(buffer.c_str(), buffer.c_str() + buffer.length(), newMap->asMap());
                vec.push_back(newMap);
                break;
            }
            case ']': {
                VariantPtr newVec = Variant::makeRepeated();
                parseVector(buffer.c_str(), buffer.c_str() + buffer.length(), newVec->asVector());
                vec.push_back(newVec);
                break;
            }
            default:
                assert(!"unk");
                break;
            }
        }
        return pBegin;
    }

    const char * parseMap(const char * pBegin, const char * pEnd, KeyValueMap & node) {
//        std::cerr << "PARSE MAP" << std::endl;
        std::string k, v;
        char dataType;
        while (pBegin < pEnd) {
            dataType = popStr(pBegin, pEnd, k);
            if (dataType == 'E')
                break;

            assert(dataType == ';');
            dataType = popStr(pBegin, pEnd, v);

            //assert(dataType == ';');

            switch (dataType) {
            case ',':
            case ';':
            case '!':
            case '#':
            case '^':
            case '~':
                node[k] = Variant::make(v.c_str(), v.length());
                break;
#if (0)
            // TODO: enable parsing of this types
            case '!':
                node[k] = Variant::make((int32_t) !v.compare("true"));
                break;
            case '#':
                node[k] = Variant::make(atoi(v.c_str()));
                break;
            case '^':
                node[k] = Variant::make(atof(v.c_str()));
                break;
#endif
            case '}': {
                VariantPtr newMap = Variant::makeMap();
                node[k] = newMap;
                parseMap(v.c_str(), v.c_str() + v.length(), newMap->asMap());
                //pBegin += v.length();
                break;
            }
            case ']': {
                VariantPtr newVec = Variant::makeRepeated();
                node[k] = newVec;
                parseVector(v.c_str(), v.c_str() + v.length(), newVec->asVector());
                //pBegin += v.length();
                break;
            }
            default:
                assert(!"unk");
                break;
            }
        }
        return pBegin;
    }

    //
    void parse(const char * pBegin, const char * pEnd, char dataType, ValuesVector & vec) {
        while (pBegin < pEnd) {
            switch (dataType) {
            case '}': {
                VariantPtr newMap = Variant::makeMap();
                pBegin = parseMap(pBegin, pEnd, newMap->asMap());
                vec.push_back(newMap);
                break;
            }
            case ']': {
                VariantPtr newVector = Variant::makeRepeated();
                pBegin = parseVector(pBegin, pEnd, newVector->asVector());
                vec.push_back(newVector);
                break;
            }
            default: break;
            }
        }
    }

    void parse(std::istream & is) {
        std::string buffer;
        char slen[10];
        size_t len = 0;

        mRoot = Variant::makeRepeated();
        for (;;) {
            // read data length
            len = 0;
            while (len < sizeof(slen)-1) {
                is.read(slen + len, 1);
                if (is.eof() || !::isdigit(slen[len])) break;
                ++len;
            }
            slen[len] = '\0';

            // nothing?
            if (len == 0) break;

            len = atoi(slen);
//            std::cerr << len << std::endl;

            // read data
            buffer.resize(len);
            char * ptr = (char*) buffer.data();
            is.read(ptr, len);
            ptr[len] = '\0';
//            std::cerr << ptr << std::endl;

            // read data type
            *slen = '\0';
            is.read(slen, 1);

            // parse
            parse(ptr, ptr + buffer.size(), *slen, mRoot->asVector());
        }
    }

    // /////////////////////////////////////////////////////////////////// //

public:

    bool isError() const {
        return !mError.empty();
    }
    const std::string & errorString() const {
        return mError;
    }

private:
    VariantPtr mRoot;
    std::string mError;
}; // MitmProxyFlow

} // namespace op
