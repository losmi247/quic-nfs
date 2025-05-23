/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: src/serialization/mount/mount.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "src/serialization/mount/mount.pb-c.h"
void mount__fhandle__init(Mount__FHandle *message) {
    static const Mount__FHandle init_value = MOUNT__FHANDLE__INIT;
    *message = init_value;
}
size_t mount__fhandle__get_packed_size(const Mount__FHandle *message) {
    assert(message->base.descriptor == &mount__fhandle__descriptor);
    return protobuf_c_message_get_packed_size((const ProtobufCMessage *)(message));
}
size_t mount__fhandle__pack(const Mount__FHandle *message, uint8_t *out) {
    assert(message->base.descriptor == &mount__fhandle__descriptor);
    return protobuf_c_message_pack((const ProtobufCMessage *)message, out);
}
size_t mount__fhandle__pack_to_buffer(const Mount__FHandle *message, ProtobufCBuffer *buffer) {
    assert(message->base.descriptor == &mount__fhandle__descriptor);
    return protobuf_c_message_pack_to_buffer((const ProtobufCMessage *)message, buffer);
}
Mount__FHandle *mount__fhandle__unpack(ProtobufCAllocator *allocator, size_t len, const uint8_t *data) {
    return (Mount__FHandle *)protobuf_c_message_unpack(&mount__fhandle__descriptor, allocator, len, data);
}
void mount__fhandle__free_unpacked(Mount__FHandle *message, ProtobufCAllocator *allocator) {
    if (!message)
        return;
    assert(message->base.descriptor == &mount__fhandle__descriptor);
    protobuf_c_message_free_unpacked((ProtobufCMessage *)message, allocator);
}
void mount__mnt_stat__init(Mount__MntStat *message) {
    static const Mount__MntStat init_value = MOUNT__MNT_STAT__INIT;
    *message = init_value;
}
size_t mount__mnt_stat__get_packed_size(const Mount__MntStat *message) {
    assert(message->base.descriptor == &mount__mnt_stat__descriptor);
    return protobuf_c_message_get_packed_size((const ProtobufCMessage *)(message));
}
size_t mount__mnt_stat__pack(const Mount__MntStat *message, uint8_t *out) {
    assert(message->base.descriptor == &mount__mnt_stat__descriptor);
    return protobuf_c_message_pack((const ProtobufCMessage *)message, out);
}
size_t mount__mnt_stat__pack_to_buffer(const Mount__MntStat *message, ProtobufCBuffer *buffer) {
    assert(message->base.descriptor == &mount__mnt_stat__descriptor);
    return protobuf_c_message_pack_to_buffer((const ProtobufCMessage *)message, buffer);
}
Mount__MntStat *mount__mnt_stat__unpack(ProtobufCAllocator *allocator, size_t len, const uint8_t *data) {
    return (Mount__MntStat *)protobuf_c_message_unpack(&mount__mnt_stat__descriptor, allocator, len, data);
}
void mount__mnt_stat__free_unpacked(Mount__MntStat *message, ProtobufCAllocator *allocator) {
    if (!message)
        return;
    assert(message->base.descriptor == &mount__mnt_stat__descriptor);
    protobuf_c_message_free_unpacked((ProtobufCMessage *)message, allocator);
}
void mount__fh_status__init(Mount__FhStatus *message) {
    static const Mount__FhStatus init_value = MOUNT__FH_STATUS__INIT;
    *message = init_value;
}
size_t mount__fh_status__get_packed_size(const Mount__FhStatus *message) {
    assert(message->base.descriptor == &mount__fh_status__descriptor);
    return protobuf_c_message_get_packed_size((const ProtobufCMessage *)(message));
}
size_t mount__fh_status__pack(const Mount__FhStatus *message, uint8_t *out) {
    assert(message->base.descriptor == &mount__fh_status__descriptor);
    return protobuf_c_message_pack((const ProtobufCMessage *)message, out);
}
size_t mount__fh_status__pack_to_buffer(const Mount__FhStatus *message, ProtobufCBuffer *buffer) {
    assert(message->base.descriptor == &mount__fh_status__descriptor);
    return protobuf_c_message_pack_to_buffer((const ProtobufCMessage *)message, buffer);
}
Mount__FhStatus *mount__fh_status__unpack(ProtobufCAllocator *allocator, size_t len, const uint8_t *data) {
    return (Mount__FhStatus *)protobuf_c_message_unpack(&mount__fh_status__descriptor, allocator, len, data);
}
void mount__fh_status__free_unpacked(Mount__FhStatus *message, ProtobufCAllocator *allocator) {
    if (!message)
        return;
    assert(message->base.descriptor == &mount__fh_status__descriptor);
    protobuf_c_message_free_unpacked((ProtobufCMessage *)message, allocator);
}
void mount__dir_path__init(Mount__DirPath *message) {
    static const Mount__DirPath init_value = MOUNT__DIR_PATH__INIT;
    *message = init_value;
}
size_t mount__dir_path__get_packed_size(const Mount__DirPath *message) {
    assert(message->base.descriptor == &mount__dir_path__descriptor);
    return protobuf_c_message_get_packed_size((const ProtobufCMessage *)(message));
}
size_t mount__dir_path__pack(const Mount__DirPath *message, uint8_t *out) {
    assert(message->base.descriptor == &mount__dir_path__descriptor);
    return protobuf_c_message_pack((const ProtobufCMessage *)message, out);
}
size_t mount__dir_path__pack_to_buffer(const Mount__DirPath *message, ProtobufCBuffer *buffer) {
    assert(message->base.descriptor == &mount__dir_path__descriptor);
    return protobuf_c_message_pack_to_buffer((const ProtobufCMessage *)message, buffer);
}
Mount__DirPath *mount__dir_path__unpack(ProtobufCAllocator *allocator, size_t len, const uint8_t *data) {
    return (Mount__DirPath *)protobuf_c_message_unpack(&mount__dir_path__descriptor, allocator, len, data);
}
void mount__dir_path__free_unpacked(Mount__DirPath *message, ProtobufCAllocator *allocator) {
    if (!message)
        return;
    assert(message->base.descriptor == &mount__dir_path__descriptor);
    protobuf_c_message_free_unpacked((ProtobufCMessage *)message, allocator);
}
void mount__name__init(Mount__Name *message) {
    static const Mount__Name init_value = MOUNT__NAME__INIT;
    *message = init_value;
}
size_t mount__name__get_packed_size(const Mount__Name *message) {
    assert(message->base.descriptor == &mount__name__descriptor);
    return protobuf_c_message_get_packed_size((const ProtobufCMessage *)(message));
}
size_t mount__name__pack(const Mount__Name *message, uint8_t *out) {
    assert(message->base.descriptor == &mount__name__descriptor);
    return protobuf_c_message_pack((const ProtobufCMessage *)message, out);
}
size_t mount__name__pack_to_buffer(const Mount__Name *message, ProtobufCBuffer *buffer) {
    assert(message->base.descriptor == &mount__name__descriptor);
    return protobuf_c_message_pack_to_buffer((const ProtobufCMessage *)message, buffer);
}
Mount__Name *mount__name__unpack(ProtobufCAllocator *allocator, size_t len, const uint8_t *data) {
    return (Mount__Name *)protobuf_c_message_unpack(&mount__name__descriptor, allocator, len, data);
}
void mount__name__free_unpacked(Mount__Name *message, ProtobufCAllocator *allocator) {
    if (!message)
        return;
    assert(message->base.descriptor == &mount__name__descriptor);
    protobuf_c_message_free_unpacked((ProtobufCMessage *)message, allocator);
}
void mount__mount_list__init(Mount__MountList *message) {
    static const Mount__MountList init_value = MOUNT__MOUNT_LIST__INIT;
    *message = init_value;
}
size_t mount__mount_list__get_packed_size(const Mount__MountList *message) {
    assert(message->base.descriptor == &mount__mount_list__descriptor);
    return protobuf_c_message_get_packed_size((const ProtobufCMessage *)(message));
}
size_t mount__mount_list__pack(const Mount__MountList *message, uint8_t *out) {
    assert(message->base.descriptor == &mount__mount_list__descriptor);
    return protobuf_c_message_pack((const ProtobufCMessage *)message, out);
}
size_t mount__mount_list__pack_to_buffer(const Mount__MountList *message, ProtobufCBuffer *buffer) {
    assert(message->base.descriptor == &mount__mount_list__descriptor);
    return protobuf_c_message_pack_to_buffer((const ProtobufCMessage *)message, buffer);
}
Mount__MountList *mount__mount_list__unpack(ProtobufCAllocator *allocator, size_t len, const uint8_t *data) {
    return (Mount__MountList *)protobuf_c_message_unpack(&mount__mount_list__descriptor, allocator, len, data);
}
void mount__mount_list__free_unpacked(Mount__MountList *message, ProtobufCAllocator *allocator) {
    if (!message)
        return;
    assert(message->base.descriptor == &mount__mount_list__descriptor);
    protobuf_c_message_free_unpacked((ProtobufCMessage *)message, allocator);
}
void mount__groups__init(Mount__Groups *message) {
    static const Mount__Groups init_value = MOUNT__GROUPS__INIT;
    *message = init_value;
}
size_t mount__groups__get_packed_size(const Mount__Groups *message) {
    assert(message->base.descriptor == &mount__groups__descriptor);
    return protobuf_c_message_get_packed_size((const ProtobufCMessage *)(message));
}
size_t mount__groups__pack(const Mount__Groups *message, uint8_t *out) {
    assert(message->base.descriptor == &mount__groups__descriptor);
    return protobuf_c_message_pack((const ProtobufCMessage *)message, out);
}
size_t mount__groups__pack_to_buffer(const Mount__Groups *message, ProtobufCBuffer *buffer) {
    assert(message->base.descriptor == &mount__groups__descriptor);
    return protobuf_c_message_pack_to_buffer((const ProtobufCMessage *)message, buffer);
}
Mount__Groups *mount__groups__unpack(ProtobufCAllocator *allocator, size_t len, const uint8_t *data) {
    return (Mount__Groups *)protobuf_c_message_unpack(&mount__groups__descriptor, allocator, len, data);
}
void mount__groups__free_unpacked(Mount__Groups *message, ProtobufCAllocator *allocator) {
    if (!message)
        return;
    assert(message->base.descriptor == &mount__groups__descriptor);
    protobuf_c_message_free_unpacked((ProtobufCMessage *)message, allocator);
}
void mount__export_list__init(Mount__ExportList *message) {
    static const Mount__ExportList init_value = MOUNT__EXPORT_LIST__INIT;
    *message = init_value;
}
size_t mount__export_list__get_packed_size(const Mount__ExportList *message) {
    assert(message->base.descriptor == &mount__export_list__descriptor);
    return protobuf_c_message_get_packed_size((const ProtobufCMessage *)(message));
}
size_t mount__export_list__pack(const Mount__ExportList *message, uint8_t *out) {
    assert(message->base.descriptor == &mount__export_list__descriptor);
    return protobuf_c_message_pack((const ProtobufCMessage *)message, out);
}
size_t mount__export_list__pack_to_buffer(const Mount__ExportList *message, ProtobufCBuffer *buffer) {
    assert(message->base.descriptor == &mount__export_list__descriptor);
    return protobuf_c_message_pack_to_buffer((const ProtobufCMessage *)message, buffer);
}
Mount__ExportList *mount__export_list__unpack(ProtobufCAllocator *allocator, size_t len, const uint8_t *data) {
    return (Mount__ExportList *)protobuf_c_message_unpack(&mount__export_list__descriptor, allocator, len, data);
}
void mount__export_list__free_unpacked(Mount__ExportList *message, ProtobufCAllocator *allocator) {
    if (!message)
        return;
    assert(message->base.descriptor == &mount__export_list__descriptor);
    protobuf_c_message_free_unpacked((ProtobufCMessage *)message, allocator);
}
static const ProtobufCFieldDescriptor mount__fhandle__field_descriptors[1] = {
    {
        "nfs_filehandle", 1, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, 0,                  /* quantifier_offset */
        offsetof(Mount__FHandle, nfs_filehandle), &nfs_fh__nfs_file_handle__descriptor, NULL, 0, /* flags */
        0, NULL, NULL /* reserved1,reserved2, etc */
    },
};
static const unsigned mount__fhandle__field_indices_by_name[] = {
    0, /* field[0] = nfs_filehandle */
};
static const ProtobufCIntRange mount__fhandle__number_ranges[1 + 1] = {{1, 0}, {0, 1}};
const ProtobufCMessageDescriptor mount__fhandle__descriptor = {
    PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
    "mount.FHandle",
    "FHandle",
    "Mount__FHandle",
    "mount",
    sizeof(Mount__FHandle),
    1,
    mount__fhandle__field_descriptors,
    mount__fhandle__field_indices_by_name,
    1,
    mount__fhandle__number_ranges,
    (ProtobufCMessageInit)mount__fhandle__init,
    NULL,
    NULL,
    NULL /* reserved[123] */
};
static const ProtobufCFieldDescriptor mount__mnt_stat__field_descriptors[1] = {
    {
        "stat", 1, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_ENUM, 0,         /* quantifier_offset */
        offsetof(Mount__MntStat, stat), &mount__stat__descriptor, NULL, 0, /* flags */
        0, NULL, NULL                                                      /* reserved1,reserved2, etc */
    },
};
static const unsigned mount__mnt_stat__field_indices_by_name[] = {
    0, /* field[0] = stat */
};
static const ProtobufCIntRange mount__mnt_stat__number_ranges[1 + 1] = {{1, 0}, {0, 1}};
const ProtobufCMessageDescriptor mount__mnt_stat__descriptor = {
    PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
    "mount.MntStat",
    "MntStat",
    "Mount__MntStat",
    "mount",
    sizeof(Mount__MntStat),
    1,
    mount__mnt_stat__field_descriptors,
    mount__mnt_stat__field_indices_by_name,
    1,
    mount__mnt_stat__number_ranges,
    (ProtobufCMessageInit)mount__mnt_stat__init,
    NULL,
    NULL,
    NULL /* reserved[123] */
};
static const ProtobufCFieldDescriptor mount__fh_status__field_descriptors[3] = {
    {
        "mnt_status", 1, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, 0,           /* quantifier_offset */
        offsetof(Mount__FhStatus, mnt_status), &mount__mnt_stat__descriptor, NULL, 0, /* flags */
        0, NULL, NULL                                                                 /* reserved1,reserved2, etc */
    },
    {
        "directory", 2, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, offsetof(Mount__FhStatus, fhstatus_body_case),
        offsetof(Mount__FhStatus, directory), &mount__fhandle__descriptor, NULL,
        0 | PROTOBUF_C_FIELD_FLAG_ONEOF, /* flags */
        0, NULL, NULL                    /* reserved1,reserved2, etc */
    },
    {
        "default_case", 3, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE,
        offsetof(Mount__FhStatus, fhstatus_body_case), offsetof(Mount__FhStatus, default_case),
        &google__protobuf__empty__descriptor, NULL, 0 | PROTOBUF_C_FIELD_FLAG_ONEOF, /* flags */
        0, NULL, NULL                                                                /* reserved1,reserved2, etc */
    },
};
static const unsigned mount__fh_status__field_indices_by_name[] = {
    2, /* field[2] = default_case */
    1, /* field[1] = directory */
    0, /* field[0] = mnt_status */
};
static const ProtobufCIntRange mount__fh_status__number_ranges[1 + 1] = {{1, 0}, {0, 3}};
const ProtobufCMessageDescriptor mount__fh_status__descriptor = {
    PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
    "mount.FhStatus",
    "FhStatus",
    "Mount__FhStatus",
    "mount",
    sizeof(Mount__FhStatus),
    3,
    mount__fh_status__field_descriptors,
    mount__fh_status__field_indices_by_name,
    1,
    mount__fh_status__number_ranges,
    (ProtobufCMessageInit)mount__fh_status__init,
    NULL,
    NULL,
    NULL /* reserved[123] */
};
static const ProtobufCFieldDescriptor mount__dir_path__field_descriptors[1] = {
    {
        "path", 1, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_STRING, 0,       /* quantifier_offset */
        offsetof(Mount__DirPath, path), NULL, &protobuf_c_empty_string, 0, /* flags */
        0, NULL, NULL                                                      /* reserved1,reserved2, etc */
    },
};
static const unsigned mount__dir_path__field_indices_by_name[] = {
    0, /* field[0] = path */
};
static const ProtobufCIntRange mount__dir_path__number_ranges[1 + 1] = {{1, 0}, {0, 1}};
const ProtobufCMessageDescriptor mount__dir_path__descriptor = {
    PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
    "mount.DirPath",
    "DirPath",
    "Mount__DirPath",
    "mount",
    sizeof(Mount__DirPath),
    1,
    mount__dir_path__field_descriptors,
    mount__dir_path__field_indices_by_name,
    1,
    mount__dir_path__number_ranges,
    (ProtobufCMessageInit)mount__dir_path__init,
    NULL,
    NULL,
    NULL /* reserved[123] */
};
static const ProtobufCFieldDescriptor mount__name__field_descriptors[1] = {
    {
        "name", 1, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_STRING, 0,    /* quantifier_offset */
        offsetof(Mount__Name, name), NULL, &protobuf_c_empty_string, 0, /* flags */
        0, NULL, NULL                                                   /* reserved1,reserved2, etc */
    },
};
static const unsigned mount__name__field_indices_by_name[] = {
    0, /* field[0] = name */
};
static const ProtobufCIntRange mount__name__number_ranges[1 + 1] = {{1, 0}, {0, 1}};
const ProtobufCMessageDescriptor mount__name__descriptor = {
    PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
    "mount.Name",
    "Name",
    "Mount__Name",
    "mount",
    sizeof(Mount__Name),
    1,
    mount__name__field_descriptors,
    mount__name__field_indices_by_name,
    1,
    mount__name__number_ranges,
    (ProtobufCMessageInit)mount__name__init,
    NULL,
    NULL,
    NULL /* reserved[123] */
};
static const ProtobufCFieldDescriptor mount__mount_list__field_descriptors[3] = {
    {
        "hostname", 1, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, 0,        /* quantifier_offset */
        offsetof(Mount__MountList, hostname), &mount__name__descriptor, NULL, 0, /* flags */
        0, NULL, NULL                                                            /* reserved1,reserved2, etc */
    },
    {
        "directory", 2, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, 0,            /* quantifier_offset */
        offsetof(Mount__MountList, directory), &mount__dir_path__descriptor, NULL, 0, /* flags */
        0, NULL, NULL                                                                 /* reserved1,reserved2, etc */
    },
    {
        "nextentry", 3, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, 0,              /* quantifier_offset */
        offsetof(Mount__MountList, nextentry), &mount__mount_list__descriptor, NULL, 0, /* flags */
        0, NULL, NULL                                                                   /* reserved1,reserved2, etc */
    },
};
static const unsigned mount__mount_list__field_indices_by_name[] = {
    1, /* field[1] = directory */
    0, /* field[0] = hostname */
    2, /* field[2] = nextentry */
};
static const ProtobufCIntRange mount__mount_list__number_ranges[1 + 1] = {{1, 0}, {0, 3}};
const ProtobufCMessageDescriptor mount__mount_list__descriptor = {
    PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
    "mount.MountList",
    "MountList",
    "Mount__MountList",
    "mount",
    sizeof(Mount__MountList),
    3,
    mount__mount_list__field_descriptors,
    mount__mount_list__field_indices_by_name,
    1,
    mount__mount_list__number_ranges,
    (ProtobufCMessageInit)mount__mount_list__init,
    NULL,
    NULL,
    NULL /* reserved[123] */
};
static const ProtobufCFieldDescriptor mount__groups__field_descriptors[2] = {
    {
        "grname", 1, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, 0,     /* quantifier_offset */
        offsetof(Mount__Groups, grname), &mount__name__descriptor, NULL, 0, /* flags */
        0, NULL, NULL                                                       /* reserved1,reserved2, etc */
    },
    {
        "grnext", 2, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, 0,       /* quantifier_offset */
        offsetof(Mount__Groups, grnext), &mount__groups__descriptor, NULL, 0, /* flags */
        0, NULL, NULL                                                         /* reserved1,reserved2, etc */
    },
};
static const unsigned mount__groups__field_indices_by_name[] = {
    0, /* field[0] = grname */
    1, /* field[1] = grnext */
};
static const ProtobufCIntRange mount__groups__number_ranges[1 + 1] = {{1, 0}, {0, 2}};
const ProtobufCMessageDescriptor mount__groups__descriptor = {
    PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
    "mount.Groups",
    "Groups",
    "Mount__Groups",
    "mount",
    sizeof(Mount__Groups),
    2,
    mount__groups__field_descriptors,
    mount__groups__field_indices_by_name,
    1,
    mount__groups__number_ranges,
    (ProtobufCMessageInit)mount__groups__init,
    NULL,
    NULL,
    NULL /* reserved[123] */
};
static const ProtobufCFieldDescriptor mount__export_list__field_descriptors[3] = {
    {
        "filesys", 1, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, 0,             /* quantifier_offset */
        offsetof(Mount__ExportList, filesys), &mount__dir_path__descriptor, NULL, 0, /* flags */
        0, NULL, NULL                                                                /* reserved1,reserved2, etc */
    },
    {
        "groups", 2, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, 0,           /* quantifier_offset */
        offsetof(Mount__ExportList, groups), &mount__groups__descriptor, NULL, 0, /* flags */
        0, NULL, NULL                                                             /* reserved1,reserved2, etc */
    },
    {
        "next", 3, PROTOBUF_C_LABEL_NONE, PROTOBUF_C_TYPE_MESSAGE, 0,                /* quantifier_offset */
        offsetof(Mount__ExportList, next), &mount__export_list__descriptor, NULL, 0, /* flags */
        0, NULL, NULL                                                                /* reserved1,reserved2, etc */
    },
};
static const unsigned mount__export_list__field_indices_by_name[] = {
    0, /* field[0] = filesys */
    1, /* field[1] = groups */
    2, /* field[2] = next */
};
static const ProtobufCIntRange mount__export_list__number_ranges[1 + 1] = {{1, 0}, {0, 3}};
const ProtobufCMessageDescriptor mount__export_list__descriptor = {
    PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
    "mount.ExportList",
    "ExportList",
    "Mount__ExportList",
    "mount",
    sizeof(Mount__ExportList),
    3,
    mount__export_list__field_descriptors,
    mount__export_list__field_indices_by_name,
    1,
    mount__export_list__number_ranges,
    (ProtobufCMessageInit)mount__export_list__init,
    NULL,
    NULL,
    NULL /* reserved[123] */
};
static const ProtobufCEnumValue mount__stat__enum_values_by_number[6] = {
    {"MNT_OK", "MOUNT__STAT__MNT_OK", 0},
    {"MNTERR_NOENT", "MOUNT__STAT__MNTERR_NOENT", 2},
    {"MNTERR_NOTEXP", "MOUNT__STAT__MNTERR_NOTEXP", 3},
    {"MNTERR_IO", "MOUNT__STAT__MNTERR_IO", 5},
    {"MNTERR_ACCES", "MOUNT__STAT__MNTERR_ACCES", 13},
    {"MNTERR_NOTDIR", "MOUNT__STAT__MNTERR_NOTDIR", 20},
};
static const ProtobufCIntRange mount__stat__value_ranges[] = {{0, 0}, {2, 1}, {5, 3}, {13, 4}, {20, 5}, {0, 6}};
static const ProtobufCEnumValueIndex mount__stat__enum_values_by_name[6] = {
    {"MNTERR_ACCES", 4},  {"MNTERR_IO", 3},     {"MNTERR_NOENT", 1},
    {"MNTERR_NOTDIR", 5}, {"MNTERR_NOTEXP", 2}, {"MNT_OK", 0},
};
const ProtobufCEnumDescriptor mount__stat__descriptor = {
    PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
    "mount.Stat",
    "Stat",
    "Mount__Stat",
    "mount",
    6,
    mount__stat__enum_values_by_number,
    6,
    mount__stat__enum_values_by_name,
    5,
    mount__stat__value_ranges,
    NULL,
    NULL,
    NULL,
    NULL /* reserved[1234] */
};
