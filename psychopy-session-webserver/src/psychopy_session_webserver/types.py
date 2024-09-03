from typing import Any, Dict, List, TypeAlias, Union
from pydantic import BaseModel, Field


ParameterDeclaration: TypeAlias = List[str]
Parameter: TypeAlias = Dict[str, Any]


class Error(BaseModel):
    title: str
    details: str


ErrorList: TypeAlias = List[Error]


class Experiment(BaseModel):
    key: str = Field(examples=["blue.psyexp", "green.psyexp"])
    resources: Dict[str, bool] = Field(
        examples=[{}, {"image.png": True, "missing.png": False}]
    )
    parameters: ParameterDeclaration = Field(
        examples=[["participant", "session"], ["participant", "session", "reward"]]
    )
    errors: ErrorList = Field(default_factory=lambda: [])


class Participant(BaseModel):
    name: str = Field(pattern=r"^[a-zA-Z0-9\-]+$", examples=["asari", "turian"])
    nextSession: int = Field(gt=0, examples=[1, 3])

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
