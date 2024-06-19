from typing import Any, ClassVar, Dict, List, TypeAlias
from pydantic import BaseModel, ConfigDict


ParameterDeclaration: TypeAlias = List[str]
Parameter: TypeAlias = Dict[str, Any]


class Experiment(BaseModel):
    key: str
    resources: Dict[str, bool]
    parameters: ParameterDeclaration


Catalog: TypeAlias = Dict[str, Experiment]
