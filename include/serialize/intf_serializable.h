
#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include <string>
#include <stdint.h>
#include "byte_stream.h"

class ISerializable
{
public:
    virtual ~ISerializable() {};
    
public:
    virtual bool Serialize(CByteStreamNetwork& bs) = 0;  
	
};

class ISerializableXML
{
public:
    virtual ~ISerializableXML() {};
    
public:
    virtual bool FromXML(const std::string& sXMLContent) = 0;  
    virtual bool ToXML(std::string& sXMLContent) = 0;  
};

#endif /* SERIALIZABLE_H */

