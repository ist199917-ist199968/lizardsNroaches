from google.protobuf.internal import containers as _containers
from google.protobuf.internal import enum_type_wrapper as _enum_type_wrapper
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Iterable as _Iterable, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class proto_direction(int, metaclass=_enum_type_wrapper.EnumTypeWrapper):
    __slots__ = ()
    UP: _ClassVar[proto_direction]
    DOWN: _ClassVar[proto_direction]
    LEFT: _ClassVar[proto_direction]
    RIGHT: _ClassVar[proto_direction]
UP: proto_direction
DOWN: proto_direction
LEFT: proto_direction
RIGHT: proto_direction

class proto_char_message(_message.Message):
    __slots__ = ("msg_type", "ch", "ncock", "direction", "cockdir", "password")
    MSG_TYPE_FIELD_NUMBER: _ClassVar[int]
    CH_FIELD_NUMBER: _ClassVar[int]
    NCOCK_FIELD_NUMBER: _ClassVar[int]
    DIRECTION_FIELD_NUMBER: _ClassVar[int]
    COCKDIR_FIELD_NUMBER: _ClassVar[int]
    PASSWORD_FIELD_NUMBER: _ClassVar[int]
    msg_type: int
    ch: str
    ncock: int
    direction: proto_direction
    cockdir: _containers.RepeatedScalarFieldContainer[proto_direction]
    password: str
    def __init__(self, msg_type: _Optional[int] = ..., ch: _Optional[str] = ..., ncock: _Optional[int] = ..., direction: _Optional[_Union[proto_direction, str]] = ..., cockdir: _Optional[_Iterable[_Union[proto_direction, str]]] = ..., password: _Optional[str] = ...) -> None: ...

class proto_display_message(_message.Message):
    __slots__ = ("posx", "posy", "ch", "score")
    POSX_FIELD_NUMBER: _ClassVar[int]
    POSY_FIELD_NUMBER: _ClassVar[int]
    CH_FIELD_NUMBER: _ClassVar[int]
    SCORE_FIELD_NUMBER: _ClassVar[int]
    posx: int
    posy: int
    ch: str
    score: int
    def __init__(self, posx: _Optional[int] = ..., posy: _Optional[int] = ..., ch: _Optional[str] = ..., score: _Optional[int] = ...) -> None: ...
