from typing import Any, Dict, List, Mapping, TypeAlias, Union
from pydantic import BaseModel


ParameterDeclaration: TypeAlias = List[str]
Parameter: TypeAlias = Dict[str, Any]


class Experiment(BaseModel):
    key: str
    resources: Dict[str, bool]
    parameters: ParameterDeclaration


class Participant(BaseModel):
    name: str
    nextSession: int

    def update(self, nextSession: int) -> bool:
        if self.nextSession >= nextSession:
            return False
        self.nextSession = nextSession
        return True


Catalog: TypeAlias = Dict[str, Experiment]

Updatable: TypeAlias = Union[
    str,
    bool,
    Mapping[str, Union[None, Experiment]],
    Mapping[str, Union[None, Participant]],
]


class UpdateEvent(BaseModel):
    type: str
    data: Updatable
