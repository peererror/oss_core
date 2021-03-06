// Library: OSS_CORE - Foundation API for SIP B2BUA
// Copyright (c) OSS Software Solutions
// Contributor: Joegen Baclor - mailto:joegen@ossapp.com
//
// Permission is hereby granted, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, execute, and to prepare
// derivative works of the Software, all subject to the
// "GNU Lesser General Public License (LGPL)".
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include <sstream>

#include "OSS/BSON/libbson.h"
#include "OSS/BSON/BSONValue.h"
#include "OSS/UTL/CoreUtils.h"


namespace OSS {
namespace BSON {
  
const int BSONValue::TYPE_STRING = BSON_TYPE_UTF8;
const int BSONValue::TYPE_INT32 = BSON_TYPE_INT32;
const int BSONValue::TYPE_INT64 = BSON_TYPE_INT64;
const int BSONValue::TYPE_BOOL = BSON_TYPE_BOOL;
const int BSONValue::TYPE_DOUBLE = BSON_TYPE_DOUBLE;
const int BSONValue::TYPE_DOCUMENT = BSON_TYPE_DOCUMENT;
const int BSONValue::TYPE_ARRAY = BSON_TYPE_ARRAY;
const int BSONValue::TYPE_UNDEFINED = BSON_TYPE_UNDEFINED;

 
BSONValue::BSONValue()
{
  _mutable = true;
  _type = TYPE_UNDEFINED;
}

BSONValue::BSONValue(const BSONValue& value)
{
  _mutable = true;
  _type = value._type;
  _value = value._value;
}

BSONValue::~BSONValue()
{
}

BSONValue& BSONValue::undefinedValue()
{
  static BSONValue undefinedType;
  undefinedType.setMutable(false);
  return undefinedType;
}

const BSONValue& BSONValue::undefinedValue() const
{
  static BSONValue undefinedType;
  undefinedType.setMutable(false);
  return undefinedType;
}

void BSONValue::swap(BSONValue& value)
{
  if (_mutable)
  {
    std::swap(_type, value._type);
    std::swap(_value, value._value);
  }
}
  
BSONValue& BSONValue::operator=(const BSONValue& value)
{
  if (_mutable)
  {
    BSONValue clone(value);
    swap(clone);
  }
  return *this;
}

void BSONValue::setValue(const BSONValue& value)
{
  if (_mutable)
  {
    _type = value._type;
    _value = value._value;
  }
}

const BSONValue& BSONValue::operator[] (const std::string& key) const
{
  if (_type == TYPE_DOCUMENT)
  {
    const Document& document = boost::any_cast<const Document&>(const_cast<BSONValue*>(this)->_value);
    Document::const_iterator iter = document.find(key);
    if (iter != document.end())
    {
      return iter->second;
    }
  }
  
  return undefinedValue();
}

BSONValue& BSONValue::operator[] (const std::string& key)
{
  if (_type == TYPE_UNDEFINED && _mutable)
  {
    _type = TYPE_DOCUMENT;
    _value = Document();
  }
  
  if (_type == TYPE_DOCUMENT)
  {
    Document& document = boost::any_cast<Document&>(_value);
    Document::iterator iter = document.find(key);
    if (iter != document.end())
    {
      return iter->second;
    }
    else
    {
      document[key] = undefinedValue();
      return document[key];
    }
  }
  return undefinedValue();
}

const BSONValue& BSONValue::operator[] (const char* key) const
{
  std::string key_(key);
  return operator[](key_);
}

BSONValue& BSONValue::operator[] (const char* key)
{
  std::string key_(key);
  return operator[](key_);
}

BSONValue& BSONValue::get(const std::string& key)
{
  if (_type == TYPE_DOCUMENT)
  {
    if (key.find(".") != std::string::npos)
    {
      Tokens tokens = OSS::string_tokenize(key, ".");
      return get(tokens);
    }
    
    Document& document = boost::any_cast<Document&>(_value);
    Document::iterator iter = document.find(key);
    if (iter != document.end())
    {
      return iter->second;
    }
  }
  return undefinedValue();
}
  
const BSONValue& BSONValue::get(const std::string& key) const
{
  if (_type == TYPE_DOCUMENT)
  {
    if (key.find(".") != std::string::npos)
    {
      Tokens tokens = OSS::string_tokenize(key, ".");
      return const_cast<BSONValue*>(this)->get(tokens);
    }
    
    const Document& document = boost::any_cast<const Document&>(_value);
    Document::const_iterator iter = document.find(key);
    if (iter != document.end())
    {
      return iter->second;
    }
  }
  return undefinedValue();
}

BSONValue& BSONValue::get(const Tokens& keys)
{
  BSONValue* currentObject = this;
  for (Tokens::const_iterator key = keys.begin(); key != keys.end(); key++)
  {
    if (currentObject->getType() == TYPE_DOCUMENT)
    {
      currentObject = &currentObject->get(*key);
      if (currentObject->getType() == TYPE_UNDEFINED)
      {
        return undefinedValue();
      }
    }
    else if (currentObject->getType() == TYPE_ARRAY)
    {
      int index = OSS::string_to_number(*key, -1);
      if (index == -1 || (unsigned)index >= currentObject->size())
      {
        return undefinedValue();
      }
      currentObject = &(*currentObject)[index];
      if (currentObject->getType() == TYPE_UNDEFINED)
      {
        return undefinedValue();
      }
    }
  }
  
  if (currentObject == this)
  {
    return undefinedValue();
  }
  
  return *currentObject;
}

BSONValue& BSONValue::get(const char* key)
{
  std::string key_(key);
  return get(key_);
}

const BSONValue& BSONValue::get(const char* key) const
{
  std::string key_(key);
  return get(key_);
}

const BSONValue& BSONValue::operator[] (std::size_t index) const
{
  if (_type == TYPE_ARRAY)
  {
    Array& array = boost::any_cast<Array&>(_value);
    
    if (index < array.size())
    {
      return array[index];
    }
    else
    {
      array.push_back(undefinedValue());
      while(index >= array.size())
      {
        array.push_back(undefinedValue());
      }
      return array[index];
    }
  }
  return undefinedValue();
}

BSONValue& BSONValue::operator[] (std::size_t index)
{ 
  if (_type == TYPE_UNDEFINED && _mutable)
  {
    _type = TYPE_ARRAY;
    _value = Array();
  }
  
  if (_type == TYPE_ARRAY)
  {
    Array& array = boost::any_cast<Array&>(_value);
    
    if (index < array.size())
    {
      return array[index];
    }
    else
    {
      array.push_back(undefinedValue());
      while(index >= array.size())
      {
        array.push_back(undefinedValue());
      }
      return array[index];
    }
  }
  return undefinedValue();
}

const BSONValue& BSONValue::operator[] (int index) const
{
  std::size_t index_ = index;
  return operator[] (index_);
}

BSONValue& BSONValue::operator[] (int index)
{
  std::size_t index_ = index;
  return operator[] (index_);
}

std::size_t BSONValue::size() const
{
  if (_type == TYPE_ARRAY)
  {
     return boost::any_cast<const Array&>(_value).size();
  }
  else if (_type == TYPE_DOCUMENT)
  {
     return boost::any_cast<const Document&>(_value).size();
  }
  return 0;
}

void BSONValue::serializeDocument(const Document& document, std::ostream& strm) const
{
  strm << "{ ";
  std::size_t size = document.size();
  std::size_t index = 0;
  for (Document::const_iterator iter = document.begin(); iter != document.end(); iter++)
  {
    switch (iter->second.getType())
    {
      case TYPE_DOCUMENT:
        strm << "\"" << iter->first << "\" : ";
        serializeDocument(iter->second.asDocument(), strm);
        break;
      case TYPE_ARRAY:
        strm << "\"" << iter->first << "\" : ";
        serializeArray(iter->second.asArray(), strm);
        break;
      case TYPE_STRING:
        strm << "\"" << iter->first << "\" : \"" << iter->second.asString() << "\"";
        break;
      case TYPE_BOOL:
        strm << "\"" << iter->first << "\" : "  << (iter->second.asBoolean() ? "true" : "false");
        break;
      case TYPE_DOUBLE:
        strm << "\"" << iter->first << "\" : "  << iter->second.asDouble();
        break;
      case TYPE_INT32:
        strm << "\"" << iter->first << "\" : "  << iter->second.asInt32();
        break;
      case TYPE_INT64:
        strm << "\"" << iter->first << "\" : "  << iter->second.asInt64();
        break;
      default:
        strm << "\"" << iter->first << "\": UndefinedType";
    }
    
    if (++index < size)
    {
      strm << ", ";
    }
  }
  strm << " }";
}

void BSONValue::serializeArray(const Array& array, std::ostream& strm) const
{
  strm << "[ ";
  std::size_t size = array.size();
  std::size_t index = 0;
  for (Array::const_iterator iter = array.begin(); iter != array.end(); iter++)
  {
    switch (iter->getType())
    {
      case TYPE_DOCUMENT:
        serializeDocument(iter->asDocument(), strm);
        break;
      case TYPE_ARRAY:
        serializeArray(iter->asArray(), strm);
        break;
      case TYPE_STRING:
        strm <<  "\"" << iter->asString() << "\"";
        break;
      case TYPE_BOOL:
        strm << (iter->asBoolean() ? "true" : "false");
        break;
      case TYPE_DOUBLE:
        strm << iter->asDouble();
        break;
      case TYPE_INT32:
        strm << iter->asInt32();
        break;
      case TYPE_INT64:
        strm << iter->asInt64();
        break;
      default:
        strm << "UndefinedType";
    }
    
    if (++index < size)
    {
      strm << ", ";
    }
  }
  strm << " ]";
}

void BSONValue::toJSON(std::ostream& strm) const
{
  switch(_type)
  {
    case TYPE_DOCUMENT:
      serializeDocument(asDocument(), strm);
      break;
    case TYPE_ARRAY:
      serializeArray(asArray(), strm);
      break;
    case TYPE_STRING:
      strm << "\"string\" : \"" << asString() << "\"";
      break;
    case TYPE_BOOL:
      strm << "\"bool\" : "  << (asBoolean() ? "true" : "false");
      break;
    case TYPE_DOUBLE:
      strm << "\"double\" : "  << asDouble();
      break;
    case TYPE_INT32:
      strm << "\"int32\" : "  << asInt32();
      break;
    case TYPE_INT64:
      strm << "\"int64\" : "  << asInt64();
      break;
  }
}

std::string BSONValue::toJSON() const
{
  std::ostringstream strm;
  toJSON(strm);
  return strm.str();
}



void BSONValue::serializeDocument(const std::string& key, const Document& document, BSONParser& bson) const
{ 
  //
  // The root document doesn't have a key
  //
  if (!key.empty())
  {
    bson.appendDocumentBegin(key);
  }
  
  for (Document::const_iterator iter = document.begin(); iter != document.end(); iter++)
  {
    switch (iter->second.getType())
    {
      case TYPE_DOCUMENT:
        serializeDocument(iter->first, iter->second.asDocument(), bson);
        break;
      case TYPE_ARRAY:
        serializeArray(iter->first, iter->second.asArray(), bson);
        break;
      case TYPE_STRING:
        bson.appendString(iter->first, iter->second.asString());
        break;
      case TYPE_BOOL:
        bson.appendBoolean(iter->first, iter->second.asBoolean());
        break;
      case TYPE_DOUBLE:
        bson.appendDouble(iter->first, iter->second.asDouble());
        break;
      case TYPE_INT32:
        bson.appendInt32(iter->first, iter->second.asInt32());
        break;
      case TYPE_INT64:
        bson.appendInt64(iter->first, iter->second.asInt64());
        break;
      default:
        bson.appendUndefined(iter->first);
        break;
    }
  }
  if (!key.empty())
  {
    bson.appendDocumentEnd(key);
  }
}

void BSONValue::serializeArray(const std::string& key, const Array& array, BSONParser& bson) const
{
  if (!key.empty())
  {
    bson.appendArrayBegin(key);
  }
  std::size_t index = 0;
  for (Array::const_iterator iter = array.begin(); iter != array.end(); iter++)
  {
    switch (iter->getType())
    {
      case TYPE_DOCUMENT:
        serializeDocument(OSS::string_from_number<std::size_t>(index), iter->asDocument(), bson);
        break;
      case TYPE_ARRAY:
        serializeArray(OSS::string_from_number<std::size_t>(index), iter->asArray(), bson);
        break;
      case TYPE_STRING:
        bson.appendString(OSS::string_from_number<std::size_t>(index), iter->asString());
        break;
      case TYPE_BOOL:
        bson.appendBoolean(OSS::string_from_number<std::size_t>(index), iter->asBoolean());
        break;
      case TYPE_DOUBLE:
        bson.appendDouble(OSS::string_from_number<std::size_t>(index), iter->asDouble());
        break;
      case TYPE_INT32:
        bson.appendInt32(OSS::string_from_number<std::size_t>(index), iter->asInt32());
        break;
      case TYPE_INT64:
        bson.appendInt64(OSS::string_from_number<std::size_t>(index), iter->asInt64());
        break;
      default:
        bson.appendUndefined(OSS::string_from_number<std::size_t>(index));
        break;
    }
    
    index++;
  }
  
  if (!key.empty())
  {
    bson.appendArrayEnd(key);
  }
}
  
void BSONValue::toBSON(BSONParser& bson) const
{
  switch(_type)
  {
    case TYPE_DOCUMENT:
      serializeDocument(std::string(), asDocument(), bson);
      break;
    case TYPE_ARRAY:
      serializeArray(std::string(), asArray(), bson);
      break;
    case TYPE_STRING:
      bson.appendString("string", asString());
      break;
    case TYPE_BOOL:
      bson.appendBoolean("bool", asBoolean());
      break;
    case TYPE_DOUBLE:
      bson.appendDouble("double", asDouble());
      break;
    case TYPE_INT32:
      bson.appendInt32("int32", asInt32());
      break;
    case TYPE_INT64:
      bson.appendInt64("int64", asInt64());
      break;
  }
}
  
BSONParser BSONValue::toBSON() const
{
  BSONParser bson;
  toBSON(bson);
  return bson;
}

void BSONValue::fromBSON(const BSONParser& bson)
{
  
}

} } // OSS::BSON




