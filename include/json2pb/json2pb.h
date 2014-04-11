/*
 * Copyright (c) 2013 Pavel Shramov <shramov@mexmat.net>
 *
 * json2pb is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef __JSON2PB_H__
#define __JSON2PB_H__

#include <string>
#include <vector>

namespace google {
namespace protobuf {
class Message;
}
}

#include <errno.h>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>


#include <stdexcept>
#include "jansson.h"

namespace {
#include "bin2ascii.h"
}

using google::protobuf::Message;
using google::protobuf::MessageFactory;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::EnumDescriptor;
using google::protobuf::EnumValueDescriptor;
using google::protobuf::Reflection;

struct json_autoptr {
        json_t * ptr;
        json_autoptr(json_t *json) : ptr(json) {}
        ~json_autoptr() { if (ptr) json_decref(ptr); }
        json_t * release() { json_t *tmp = ptr; ptr = 0; return tmp; }
};

class j2pb_error : public std::exception {
        std::string _error;
public:
        j2pb_error(const std::string &e) : _error(e) {}
        j2pb_error(const FieldDescriptor *field, const std::string &e) : _error(field->name() + ": " + e) {}
        virtual ~j2pb_error() throw() {};

        virtual const char *what() const throw () { return _error.c_str(); };
};

void json2pb(google::protobuf::Message &msg, const char *buf, size_t size);
void json2pb_force(google::protobuf::Message &msg, json_t *root);
void json2pb_ex(google::protobuf::Message &msg, google::protobuf::Message &sub_msg, const char *buf, size_t size);
void json2pb_ignore_field(google::protobuf::Message& msg, const char *ignore_field_name, const char *buf, size_t size);
std::string pb2json(const google::protobuf::Message &msg);
std::string pb2json_ex(const google::protobuf::Message &msg, const google::protobuf::Message &sub_msg);
std::string pb2json_insert_field(const google::protobuf::Message &msg, std::string &key, std::string &vale);
std::string pb2json_insert_msg(const std::string *keys, 
							   const std::string *values,
							   int size,
							   const std::string &msg_key, 
		                       const google::protobuf::Message &msg,
							   const std::string &key,
							   const std::string &value);
#endif//__JSON2PB_H__
