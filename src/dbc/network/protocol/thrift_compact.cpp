#include "thrift_compact.h"
#include <limits>
#include "protocol.h"

//#include "thrift/config.h"

#define ARITHMETIC_RIGHT_SHIFT 1
#define SIGNED_RIGHT_SHIFT_IS  1

/*
 * TCompactProtocol::i*ToZigzag depend on the fact that the right shift
 * operator on a signed integer is an arithmetic (sign-extending) shift.
 * If this is not the case, the current implementation will not work.
 * If anyone encounters this error, we can try to figure out the best
 * way to implement an arithmetic right shift on their platform.
 */
#if !defined(SIGNED_RIGHT_SHIFT_IS) || !defined(ARITHMETIC_RIGHT_SHIFT)
# error "Unable to determine the behavior of a signed right shift"
#endif
#if SIGNED_RIGHT_SHIFT_IS != ARITHMETIC_RIGHT_SHIFT
# error "TCompactProtocol currently only works if a signed right shift is arithmetic"
#endif

#ifdef __GNUC__
#define UNLIKELY(val) (__builtin_expect((val), 0))
#else
#define UNLIKELY(val) (val)
#endif

namespace network 
{
        namespace detail 
        {
            namespace compact
            {
                enum Types
                {
                    CT_STOP = 0x7F,
                    CT_BOOLEAN_TRUE = 0x01,
                    CT_BOOLEAN_FALSE = 0x02,
                    CT_BYTE = 0x03,
                    CT_I16 = 0x04,
                    CT_I32 = 0x05,
                    CT_I64 = 0x06,
                    CT_DOUBLE = 0x07,
                    CT_BINARY = 0x08,
                    CT_LIST = 0x09,
                    CT_SET = 0x0A,
                    CT_MAP = 0x0B,
                    CT_STRUCT = 0x0C
                };

                const int8_t TTypeToCType[128] = {
                        0, //CT_STOP, // T_STOP
                        0, // unused
                        CT_BOOLEAN_TRUE, // T_BOOL
                        CT_BYTE, // T_BYTE
                        CT_DOUBLE, // T_DOUBLE
                        0, // unused
                        CT_I16, // T_I16
                        0, // unused
                        CT_I32, // T_I32
                        0, // unused
                        CT_I64, // T_I64
                        CT_BINARY, // T_STRING
                        CT_STRUCT, // T_STRUCT
                        CT_MAP, // T_MAP
                        CT_SET, // T_SET
                        CT_LIST, // T_LIST
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, CT_STOP //0x7F
                };
            }
        }

//        using namespace apache::thrift::protocol;
    using ::apache::thrift::protocol::TProtocolException;

    uint32_t compact_protocol::writeMessageBegin(
            const std::string& name,
            const TMessageType messageType,
            const int32_t seqid) {

        return 0;
    }

    /**
    * Write a field header containing the field id and field type. If the
    * difference between the current field id and the last one is small (< 15),
    * then the field id will be encoded in the 4 MSB as a delta. Otherwise, the
    * field id will follow the type header as a zigzag varint.
    */
    uint32_t compact_protocol::writeFieldBegin(const char* name,
                                                            const TType fieldType,
                                                            const int16_t fieldId) {
        if (fieldType == T_BOOL) {
            booleanField_.name = name;
            booleanField_.fieldType = fieldType;
            booleanField_.fieldId = fieldId;
        } else {
            return writeFieldBeginInternal(name, fieldType, fieldId, -1);
        }
        return 0;
    }

    /**
    * Write the STOP symbol so we know there are no more fields in this struct.
    */    
    uint32_t compact_protocol::writeFieldStop() {
        return writeByte(T_STOP);
    }

    /**
    * Write a struct begin. This doesn't actually put anything on the wire. We
    * use it as an opportunity to put special placeholder markers on the field
    * stack so we can get the field id deltas correct.
    */    
    uint32_t compact_protocol::writeStructBegin(const char* name) {
        (void) name;
        lastField_.push(lastFieldId_);
        lastFieldId_ = 0;
        return 0;
    }

    /**
    * Write a struct end. This doesn't actually put anything on the wire. We use
    * this as an opportunity to pop the last field from the current struct off
    * of the field stack.
    */
    uint32_t compact_protocol::writeStructEnd() {
        lastFieldId_ = lastField_.top();
        lastField_.pop();
        return 0;
    }

    /**
    * Write a List header.
    */
    uint32_t compact_protocol::writeListBegin(const TType elemType,
                                                            const uint32_t size) {
        return writeCollectionBegin(elemType, size);
    }

    /**
    * Write a set header.
    */
    uint32_t compact_protocol::writeSetBegin(const TType elemType,
                                                            const uint32_t size) {
        return writeCollectionBegin(elemType, size);
    }

    /**
    * Write a map header. If the map is empty, omit the key and value type
    * headers, as we don't need any additional information to skip it.
    */
    uint32_t compact_protocol::writeMapBegin(const TType keyType,
                                                            const TType valType,
                                                            const uint32_t size) {
        uint32_t wsize = 0;

        if (size == 0) {
            wsize += writeByte(0);
        } else {
            wsize += writeVarint32(size);
            wsize += writeByte(getCompactType(keyType) << 4 | getCompactType(valType));
        }
        return wsize;
    }

    /**
    * Write a boolean value. Potentially, this could be a boolean field, in
    * which case the field header info isn't written yet. If so, decide what the
    * right type header is for the value and then write the field header.
    * Otherwise, write a single byte.
    */
    uint32_t compact_protocol::writeBool(const bool value) {
        uint32_t wsize = 0;

        if (booleanField_.name != NULL) {
            // we haven't written the field header yet
            wsize
                    += writeFieldBeginInternal(booleanField_.name,
                                                booleanField_.fieldType,
                                                booleanField_.fieldId,
                                                static_cast<int8_t>(value
                                                                    ? detail::compact::CT_BOOLEAN_TRUE
                                                                    : detail::compact::CT_BOOLEAN_FALSE));
            booleanField_.name = NULL;
        } else {
            // we're not part of a field, so just write the value
            wsize
                    += writeByte(static_cast<int8_t>(value
                                                        ? detail::compact::CT_BOOLEAN_TRUE
                                                        : detail::compact::CT_BOOLEAN_FALSE));
        }
        return wsize;
    }


    uint32_t compact_protocol::writeByte(const int8_t byte) {
        m_buf->write_to_byte_buf((char*)&byte, 1);
        return 1;
    }

    /**
    * Write an i16 as a zigzag varint.
    */
    uint32_t compact_protocol::writeI16(const int16_t i16) {
        return writeVarint32(i32ToZigzag(i16));
    }

    /**
    * Write an i32 as a zigzag varint.
    */
    uint32_t compact_protocol::writeI32(const int32_t i32) {
        return writeVarint32(i32ToZigzag(i32));
    }

    /**
    * Write an i64 as a zigzag varint.
    */

    uint32_t compact_protocol::writeI64(const int64_t i64) {
        return writeVarint64(i64ToZigzag(i64));
    }

    /**
    * Write a double to the wire as 8 bytes.
    */

    uint32_t compact_protocol::writeDouble(const double dub) {
        BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
        BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

        uint64_t bits = bitwise_cast<uint64_t>(dub);
        bits = byte_order::hton64(bits);
        m_buf->write_to_byte_buf((char*)&bits, 8);
        return 8;
    }

    /**
    * Write a string to the wire with a varint size preceding.
    */

    uint32_t compact_protocol::writeString(const std::string& str) {
        return writeBinary(str);
    }

    uint32_t compact_protocol::writeBinary(const std::string& str) {
        if(str.size() > (std::numeric_limits<uint32_t>::max)())
            throw TProtocolException(TProtocolException::SIZE_LIMIT);
        uint32_t ssize = static_cast<uint32_t>(str.size());
        uint32_t wsize = writeVarint32(ssize) ;
        // checking ssize + wsize > uint_max, but we don't want to overflow while checking for overflows.
        // transforming the check to ssize > uint_max - wsize
        if(ssize > (std::numeric_limits<uint32_t>::max)() - wsize)
            throw TProtocolException(TProtocolException::SIZE_LIMIT);
        wsize += ssize;
        if (ssize > 0)
        {
            m_buf->write_to_byte_buf((char *) str.data(), ssize);
        }
        return wsize;
    }

    //
    // Internal Writing methods
    //

    /**
    * The workhorse of writeFieldBegin. It has the option of doing a
    * 'type override' of the type header. This is used specifically in the
    * boolean field case.
    */

    int32_t compact_protocol::writeFieldBeginInternal(
            const char* name,
            const TType fieldType,
            const int16_t fieldId,
            int8_t typeOverride) {
        (void) name;
        uint32_t wsize = 0;

        // if there's a type override, use that.
        int8_t typeToWrite = (typeOverride == -1 ? getCompactType(fieldType) : typeOverride);

        // check if we can use delta encoding for the field id
        if (fieldId > lastFieldId_ && fieldId - lastFieldId_ <= 15) {
            // write them together
            wsize += writeByte(static_cast<int8_t>((fieldId - lastFieldId_)
                                                            << 4 | typeToWrite));
        } else {
            // write them separate
            wsize += writeByte(typeToWrite);
            wsize += writeI16(fieldId);
        }

        lastFieldId_ = fieldId;
        return wsize;
    }

    /**
    * Abstract method for writing the start of lists and sets. List and sets on
    * the wire differ only by the type indicator.
    */

    uint32_t compact_protocol::writeCollectionBegin(const TType elemType,
                                                    int32_t size) {
        uint32_t wsize = 0;
        if (size <= 14) {
            wsize += writeByte(static_cast<int8_t>(size
                                                            << 4 | getCompactType(elemType)));
        } else {
            wsize += writeByte(0xf0 | getCompactType(elemType));
            wsize += writeVarint32(size);
        }
        return wsize;
    }

    /**
    * Write an i32 as a varint. Results in 1-5 bytes on the wire.
    */

    uint32_t compact_protocol::writeVarint32(uint32_t n) {
        uint8_t buf[5];
        uint32_t wsize = 0;

        while (true) {
            if ((n & ~0x7F) == 0) {
                buf[wsize++] = (int8_t)n;
                break;
            } else {
                buf[wsize++] = (int8_t)((n & 0x7F) | 0x80);
                n >>= 7;
            }
        }
        m_buf->write_to_byte_buf((char*)buf, wsize);
        return wsize;
    }

    /**
    * Write an i64 as a varint. Results in 1-10 bytes on the wire.
    */
    uint32_t compact_protocol::writeVarint64(uint64_t n) {
        uint8_t buf[10];
        uint32_t wsize = 0;

        while (true) {
            if ((n & ~0x7FL) == 0) {
                buf[wsize++] = (int8_t)n;
                break;
            } else {
                buf[wsize++] = (int8_t)((n & 0x7F) | 0x80);
                n >>= 7;
            }
        }
        m_buf->write_to_byte_buf((char*)buf, wsize);
        return wsize;
    }

    /**
    * Convert l into a zigzag long. This allows negative numbers to be
    * represented compactly as a varint.
    */
    uint64_t compact_protocol::i64ToZigzag(const int64_t l) {
        return (static_cast<uint64_t>(l) << 1) ^ (l >> 63);
    }

    /**
    * Convert n into a zigzag int. This allows negative numbers to be
    * represented compactly as a varint.
    */
    uint32_t compact_protocol::i32ToZigzag(const int32_t n) {
        return (static_cast<uint32_t>(n) << 1) ^ (n >> 31);
    }

    /**
    * Given a TType value, find the appropriate detail::compact::Types value
    */
    int8_t compact_protocol::getCompactType(const TType ttype) {
        return detail::compact::TTypeToCType[ttype];
    }

    //
    // Reading Methods
    //

    /**
    * Read a message header.
    */
    uint32_t compact_protocol::readMessageBegin(
            std::string& name,
            TMessageType& messageType,
            int32_t& seqid) {
        return 0;
    }

    /**
    * Read a struct begin. There's nothing on the wire for this, but it is our
    * opportunity to push a new struct begin marker on the field stack.
    */
    uint32_t compact_protocol::readStructBegin(std::string& name) {
        name = "";
        lastField_.push(lastFieldId_);
        lastFieldId_ = 0;
        return 0;
    }

    /**
    * Doesn't actually consume any wire data, just removes the last field for
    * this struct from the field stack.
    */
    uint32_t compact_protocol::readStructEnd() {
        lastFieldId_ = lastField_.top();
        lastField_.pop();
        return 0;
    }

    /**
    * Read a field header off the wire.
    */
    uint32_t compact_protocol::readFieldBegin(std::string& name,
                                                            TType& fieldType,
                                                            int16_t& fieldId) {
        (void) name;
        uint32_t rsize = 0;
        int8_t byte;
        int8_t type;

        rsize += readByte(byte);
        type = (byte & 0x0f);   //dbc: T_STOP is 0x7F and occupy 8 bites

        // if it's a stop, then we can return immediately, as the struct is over.
        if ( (byte & 0xff) == T_STOP) {
            fieldType = T_STOP;
            fieldId = 0;
            return rsize;
        }

        // mask off the 4 MSB of the type header. it could contain a field id delta.
        int16_t modifier = (int16_t)(((uint8_t)byte & 0xf0) >> 4);
        if (modifier == 0) {
            // not a delta, look ahead for the zigzag varint field id.
            rsize += readI16(fieldId);
        } else {
            fieldId = (int16_t)(lastFieldId_ + modifier);
        }
        fieldType = getTType(type);

        // if this happens to be a boolean field, the value is encoded in the type
        if (type == detail::compact::CT_BOOLEAN_TRUE ||
            type == detail::compact::CT_BOOLEAN_FALSE) {
            // save the boolean value in a special instance variable.
            boolValue_.hasBoolValue = true;
            boolValue_.boolValue =
                    (type == detail::compact::CT_BOOLEAN_TRUE ? true : false);
        }

        // push the new field onto the field stack so we can keep the deltas going.
        lastFieldId_ = fieldId;
        return rsize;
    }

    /**
    * Read a map header off the wire. If the size is zero, skip reading the key
    * and value type. This means that 0-length maps will yield TMaps without the
    * "correct" types.
    */
    uint32_t compact_protocol::readMapBegin(TType& keyType,
                                                            TType& valType,
                                                            uint32_t& size) {
        uint32_t rsize = 0;
        int8_t kvType = 0;
        int32_t msize = 0;

        rsize += readVarint32(msize);
        if (msize != 0)
            rsize += readByte(kvType);

        if (msize < 0) {
            throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
        } else if (container_limit_ && msize > container_limit_) {
            throw TProtocolException(TProtocolException::SIZE_LIMIT);
        }

        keyType = getTType((int8_t)((uint8_t)kvType >> 4));
        valType = getTType((int8_t)((uint8_t)kvType & 0xf));
        size = (uint32_t)msize;

        return rsize;
    }

    /**
    * Read a list header off the wire. If the list size is 0-14, the size will
    * be packed into the element type header. If it's a longer list, the 4 MSB
    * of the element type header will be 0xF, and a varint will follow with the
    * true size.
    */
    uint32_t compact_protocol::readListBegin(TType& elemType,
                                                            uint32_t& size) {
        int8_t size_and_type;
        uint32_t rsize = 0;
        int32_t lsize;

        rsize += readByte(size_and_type);

        lsize = ((uint8_t)size_and_type >> 4) & 0x0f;
        if (lsize == 15) {
            rsize += readVarint32(lsize);
        }

        if (lsize < 0) {
            throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
        } else if (container_limit_ && lsize > container_limit_) {
            throw TProtocolException(TProtocolException::SIZE_LIMIT);
        }

        elemType = getTType((int8_t)(size_and_type & 0x0f));
        size = (uint32_t)lsize;

        return rsize;
    }

    /**
    * Read a set header off the wire. If the set size is 0-14, the size will
    * be packed into the element type header. If it's a longer set, the 4 MSB
    * of the element type header will be 0xF, and a varint will follow with the
    * true size.
    */
    uint32_t compact_protocol::readSetBegin(TType& elemType,
                                                            uint32_t& size) {
        return readListBegin(elemType, size);
    }

    /**
    * Read a boolean off the wire. If this is a boolean field, the value should
    * already have been read during readFieldBegin, so we'll just consume the
    * pre-stored value. Otherwise, read a byte.
    */
    uint32_t compact_protocol::readBool(bool& value) {
        if (boolValue_.hasBoolValue == true) {
            value = boolValue_.boolValue;
            boolValue_.hasBoolValue = false;
            return 0;
        } else {
            int8_t val;
            readByte(val);
            value = (val == detail::compact::CT_BOOLEAN_TRUE);
            return 1;
        }
    }

    /**
    * Read a single byte off the wire. Nothing interesting here.
    */
    uint32_t compact_protocol::readByte(int8_t& byte) {
        uint8_t b[1];
        m_buf->read_from_byte_buf((char*)b, 1);
        byte = *(int8_t*)b;
        return 1;
    }

    /**
    * Read an i16 from the wire as a zigzag varint.
    */
    uint32_t compact_protocol::readI16(int16_t& i16) {
        int32_t value;
        uint32_t rsize = readVarint32(value);
        i16 = (int16_t)zigzagToI32(value);
        return rsize;
    }

    /**
    * Read an i32 from the wire as a zigzag varint.
    */
    uint32_t compact_protocol::readI32(int32_t& i32) {
        int32_t value;
        uint32_t rsize = readVarint32(value);
        i32 = zigzagToI32(value);
        return rsize;
    }

    /**
    * Read an i64 from the wire as a zigzag varint.
    */
    uint32_t compact_protocol::readI64(int64_t& i64) {
        int64_t value;
        uint32_t rsize = readVarint64(value);
        i64 = zigzagToI64(value);
        return rsize;
    }

    /**
    * No magic here - just read a double off the wire.
    */
    uint32_t compact_protocol::readDouble(double& dub) {
        BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
        BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

        union {
            uint64_t bits;
            uint8_t b[8];
        } u;
        m_buf->read_from_byte_buf((char*)u.b, 8);
        u.bits = byte_order::ntoh64(u.bits);
        dub = bitwise_cast<double>(u.bits);
        return 8;
    }

    uint32_t compact_protocol::readString(std::string& str) {
        return readBinary(str);
    }

    /**
    * Read a byte[] from the wire.
    */
    uint32_t compact_protocol::readBinary(std::string& str) {
        int32_t rsize = 0;
        int32_t size;

        rsize += readVarint32(size);
        // Catch empty string case
        if (size == 0) {
            str = "";
            return rsize;
        }

        // Catch error cases
        if (size < 0) {
            throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
        }
        if (string_limit_ > 0 && size > string_limit_) {
            throw TProtocolException(TProtocolException::SIZE_LIMIT);
        }

        // Use the heap here to prevent stack overflow for v. large strings
        if (size > string_buf_size_ || string_buf_ == NULL) {
            void* new_string_buf = std::realloc(string_buf_, (uint32_t)size);
            if (new_string_buf == NULL) {
                throw std::bad_alloc();
            }
            string_buf_ = (uint8_t*)new_string_buf;
            string_buf_size_ = size;
        }
        m_buf->read_from_byte_buf((char*)string_buf_, size);
        str.assign((char*)string_buf_, size);

        return rsize + (uint32_t)size;
    }

    /**
    * Read an i32 from the wire as a varint. The MSB of each byte is set
    * if there is another byte to follow. This can read up to 5 bytes.
    */
    uint32_t compact_protocol::readVarint32(int32_t& i32) {
        int64_t val;
        uint32_t rsize = readVarint64(val);
        i32 = (int32_t)val;
        return rsize;
    }

    /**
    * Read an i64 from the wire as a proper varint. The MSB of each byte is set
    * if there is another byte to follow. This can read up to 10 bytes.
    */
    uint32_t compact_protocol::readVarint64(int64_t& i64) {
        uint32_t rsize = 0;
        uint64_t val = 0;
        int shift = 0;
        uint8_t buf[10];  // 64 bits / (7 bits/byte) = 10 bytes.
//            uint32_t buf_size = sizeof(buf);
//            const uint8_t* borrowed = trans_->borrow(buf, &buf_size);
//
//            // Fast path.
//            if (borrowed != NULL) {
//                while (true) {
//                    uint8_t byte = borrowed[rsize];
//                    rsize++;
//                    val |= (uint64_t)(byte & 0x7f) << shift;
//                    shift += 7;
//                    if (!(byte & 0x80)) {
//                        i64 = val;
//                        trans_->consume(rsize);
//                        return rsize;
//                    }
//                    // Have to check for invalid data so we don't crash.
//                    if (UNLIKELY(rsize == sizeof(buf))) {
//                        throw TProtocolException(TProtocolException::INVALID_DATA, "Variable-length int over 10 bytes.");
//                    }
//                }
//            }
//
//                // Slow path.
//            else {
            while (true) {
                uint8_t byte;
                rsize += m_buf->read_from_byte_buf((char*)&byte, 1);
                val |= (uint64_t)(byte & 0x7f) << shift;
                shift += 7;
                if (!(byte & 0x80)) {
                    i64 = val;
                    return rsize;
                }
                // Might as well check for invalid data on the slow path too.
                if (UNLIKELY(rsize >= sizeof(buf))) {
                    throw TProtocolException(TProtocolException::INVALID_DATA, "Variable-length int over 10 bytes.");
                }
            }
//            }
    }

    /**
    * Convert from zigzag int to int.
    */
    int32_t compact_protocol::zigzagToI32(uint32_t n) {
        return (n >> 1) ^ static_cast<uint32_t>(-static_cast<int32_t>(n & 1));
    }

    /**
    * Convert from zigzag long to long.
    */
    int64_t compact_protocol::zigzagToI64(uint64_t n) {
        return (n >> 1) ^ static_cast<uint64_t>(-static_cast<int64_t>(n & 1));
    }

    TType compact_protocol::getTType(int8_t type) {
        switch (type) {
            case T_STOP:
                return T_STOP;
            case detail::compact::CT_BOOLEAN_FALSE:
            case detail::compact::CT_BOOLEAN_TRUE:
                return T_BOOL;
            case detail::compact::CT_BYTE:
                return T_BYTE;
            case detail::compact::CT_I16:
                return T_I16;
            case detail::compact::CT_I32:
                return T_I32;
            case detail::compact::CT_I64:
                return T_I64;
            case detail::compact::CT_DOUBLE:
                return T_DOUBLE;
            case detail::compact::CT_BINARY:
                return T_STRING;
            case detail::compact::CT_LIST:
                return T_LIST;
            case detail::compact::CT_SET:
                return T_SET;
            case detail::compact::CT_MAP:
                return T_MAP;
            case detail::compact::CT_STRUCT:
                return T_STRUCT;
            default:
                throw TProtocolException(std::string("don't know what type: ") + (char)type);
        }
    }

    uint32_t compact_protocol::skip(TType type) {
        return network::skip(*this, type);
    }

}



