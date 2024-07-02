from typing import Any, Dict, List, Mapping, TypeAlias, Union
from pydantic import BaseModel, Field


ParameterDeclaration: TypeAlias = List[str]
Parameter: TypeAlias = Dict[str, Any]


class Experiment(BaseModel):
    key: str
    resources: Dict[str, bool]
    parameters: ParameterDeclaration


class Participant(BaseModel):
    name: str = Field(pattern=r"^[a-zA-Z0-9\-]+$")
    nextSession: int = Field(gt=0)

    def update(self, nextSession: int) -> bool:
        if self.nextSession >= nextSession:
            return False
        self.nextSession = nextSession
        return True


Catalog: TypeAlias = Dict[str, Experiment]

Updatable: TypeAlias = Union[
    str,
    bool,
    Dict[str, Union[None, Experiment]],
    Dict[str, Union[None, Participant]],
]


class UpdateEvent(BaseModel):
    type: str
    data: Updatable
