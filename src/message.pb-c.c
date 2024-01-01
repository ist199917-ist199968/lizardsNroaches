/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: message.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "message.pb-c.h"
void   proto_char_message__init
                     (ProtoCharMessage         *message)
{
  static const ProtoCharMessage init_value = PROTO_CHAR_MESSAGE__INIT;
  *message = init_value;
}
size_t proto_char_message__get_packed_size
                     (const ProtoCharMessage *message)
{
  assert(message->base.descriptor == &proto_char_message__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t proto_char_message__pack
                     (const ProtoCharMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &proto_char_message__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t proto_char_message__pack_to_buffer
                     (const ProtoCharMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &proto_char_message__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
ProtoCharMessage *
       proto_char_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (ProtoCharMessage *)
     protobuf_c_message_unpack (&proto_char_message__descriptor,
                                allocator, len, data);
}
void   proto_char_message__free_unpacked
                     (ProtoCharMessage *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &proto_char_message__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   proto_display_message__init
                     (ProtoDisplayMessage         *message)
{
  static const ProtoDisplayMessage init_value = PROTO_DISPLAY_MESSAGE__INIT;
  *message = init_value;
}
size_t proto_display_message__get_packed_size
                     (const ProtoDisplayMessage *message)
{
  assert(message->base.descriptor == &proto_display_message__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t proto_display_message__pack
                     (const ProtoDisplayMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &proto_display_message__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t proto_display_message__pack_to_buffer
                     (const ProtoDisplayMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &proto_display_message__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
ProtoDisplayMessage *
       proto_display_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (ProtoDisplayMessage *)
     protobuf_c_message_unpack (&proto_display_message__descriptor,
                                allocator, len, data);
}
void   proto_display_message__free_unpacked
                     (ProtoDisplayMessage *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &proto_display_message__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor proto_char_message__field_descriptors[6] =
{
  {
    "msg_type",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(ProtoCharMessage, msg_type),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "ch",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(ProtoCharMessage, ch),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "ncock",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(ProtoCharMessage, ncock),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "direction",
    4,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    offsetof(ProtoCharMessage, direction),
    &proto_direction__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "cockdir",
    5,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_ENUM,
    offsetof(ProtoCharMessage, n_cockdir),
    offsetof(ProtoCharMessage, cockdir),
    &proto_direction__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "password",
    6,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(ProtoCharMessage, password),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned proto_char_message__field_indices_by_name[] = {
  1,   /* field[1] = ch */
  4,   /* field[4] = cockdir */
  3,   /* field[3] = direction */
  0,   /* field[0] = msg_type */
  2,   /* field[2] = ncock */
  5,   /* field[5] = password */
};
static const ProtobufCIntRange proto_char_message__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 6 }
};
const ProtobufCMessageDescriptor proto_char_message__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "proto_char_message",
  "ProtoCharMessage",
  "ProtoCharMessage",
  "",
  sizeof(ProtoCharMessage),
  6,
  proto_char_message__field_descriptors,
  proto_char_message__field_indices_by_name,
  1,  proto_char_message__number_ranges,
  (ProtobufCMessageInit) proto_char_message__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor proto_display_message__field_descriptors[4] =
{
  {
    "posx",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(ProtoDisplayMessage, posx),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "posy",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(ProtoDisplayMessage, posy),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "ch",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(ProtoDisplayMessage, ch),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "score",
    4,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(ProtoDisplayMessage, score),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned proto_display_message__field_indices_by_name[] = {
  2,   /* field[2] = ch */
  0,   /* field[0] = posx */
  1,   /* field[1] = posy */
  3,   /* field[3] = score */
};
static const ProtobufCIntRange proto_display_message__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor proto_display_message__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "proto_display_message",
  "ProtoDisplayMessage",
  "ProtoDisplayMessage",
  "",
  sizeof(ProtoDisplayMessage),
  4,
  proto_display_message__field_descriptors,
  proto_display_message__field_indices_by_name,
  1,  proto_display_message__number_ranges,
  (ProtobufCMessageInit) proto_display_message__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue proto_direction__enum_values_by_number[4] =
{
  { "UP", "PROTO_DIRECTION__UP", 0 },
  { "DOWN", "PROTO_DIRECTION__DOWN", 1 },
  { "LEFT", "PROTO_DIRECTION__LEFT", 2 },
  { "RIGHT", "PROTO_DIRECTION__RIGHT", 3 },
};
static const ProtobufCIntRange proto_direction__value_ranges[] = {
{0, 0},{0, 4}
};
static const ProtobufCEnumValueIndex proto_direction__enum_values_by_name[4] =
{
  { "DOWN", 1 },
  { "LEFT", 2 },
  { "RIGHT", 3 },
  { "UP", 0 },
};
const ProtobufCEnumDescriptor proto_direction__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "proto_direction",
  "proto_direction",
  "ProtoDirection",
  "",
  4,
  proto_direction__enum_values_by_number,
  4,
  proto_direction__enum_values_by_name,
  1,
  proto_direction__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
