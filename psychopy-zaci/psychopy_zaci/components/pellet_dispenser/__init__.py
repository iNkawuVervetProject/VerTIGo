from contextlib import contextmanager
from pathlib import Path

from psychopy.experiment.components import BaseComponent, Param


# TODO: actually translate
def _translate(v):
    return v


class _BufferContext:
    def __init__(self, buff):
        self.buff = buff

    def write(self, lines):
        self.buff.writeIndentedLines(lines)

    def add_indent(self, value=1):
        self.buff.setIndentLevel(value, relative=True)

    def remove_indent(self, value=1):
        self.buff.setIndentLevel(-value, relative=True)

    @contextmanager
    def indent(self):
        self.add_indent()
        yield
        self.remove_indent()


class PelletDispenserComponent(BaseComponent):
    """Delivers a pellet reward to the animal and monitor dispensed quantities"""

    targets = ["PsychoPy"]
    categories = ["I/O"]
    iconFile = Path(__file__).parent / "reward.png"
    tooltip = _translate(
        "Pellet Dispenser: Delivers a pellet reward to the animal and monitor"
        " dispensed quantities"
    )

    def __init__(
        self,
        exp,
        parentName,
        name="reward",
        count=1,
        startType="time (s)",
        startVal="0.0",
        stopType="time (s)",
        stopVal="",
        saveStats=True,
        startEstim="",
        durationEstim="",
        endRoutineOnDispense=True,
    ):
        super(PelletDispenserComponent, self).__init__(
            exp,
            parentName,
            name,
            startType=startType,
            startVal=startVal,
            stopType=stopType,
            stopVal=stopVal,
            startEstim=startEstim,
            durationEstim=durationEstim,
        )

        self.type = "PelletDispenser"

        self.url = Path(__file__).parent / "pellet_dispenser.html"

        self.order += [
            "endRoutineOnDispense",
            "count",  # Basic tab
            "saveStats",  # Data tab
        ]

        self.params["count"] = Param(
            count,
            categ="Basic",
            valType="num",
            allowedTypes=[],
            inputType="single",
            updates="constant",
            allowedUpdates=["constant", "set every repeat"],
            hint=_translate(
                "The number of pellet to dispense. This may not the number of"
                " pellet actually dispensed, but threshold after wich the"
                " dispenser will stop dispense pellets."
            ),
            label=_translate("Count"),
        )

        self.params["endRoutineOnDispense"] = Param(
            endRoutineOnDispense,
            categ="Basic",
            valType="bool",
            inputType="bool",
            updates="constant",
            hint=_translate(
                "Should the dispensing completion force the end of the routine"
            ),
            label=_translate("End Routine after dispense"),
        )

        self.params["saveStats"] = Param(
            saveStats,
            categ="Data",
            valType="bool",
            updates="constant",
            hint=_translate(
                "Should the number of dispensed pellet be recorded in the"
                " result files"
            ),
            label=_translate("Save number of dispensed pellets"),
        )

    def writeInitCode(self, buff):
        tmplVars = {"name": self.params["name"]}
        # fmt: off
        code = (
            "from psychopy_zaci.devices.pellet_dispenser import PelletDispenserDevice, PelletDispenserError\n"
            "\n"
            "{name} = PelletDispenserDevice(win,logging)\n"
            "\n"
        )
        # fmt: on
        buff.writeIndentedLines(code.format(**tmplVars))

    def writeRoutineStartCode(self, _buff):
        super().writeRoutineStartCode(_buff)
        if self.params["count"].updates != "constant":
            return

        buff = _BufferContext(_buff)
        buff.write(
            "# *%(name)s* routine start\n%(name)s.count = None\n" % self.params
        )

    def writeFrameCode(self, _buff):
        buff = _BufferContext(_buff)

        tmplVars = {
            "name": self.params["name"],
            "count": self.params["count"].val,
        }

        endRoutine = self.params["endRoutineOnDispense"].val

        buff.write("# *{name}* updates\n".format(**tmplVars))
        indented = self.writeStartTestCode(_buff)
        if indented:
            if self.params["count"].updates == "constant":
                buff.write("{name}.dispense({count})\n".format(**tmplVars))
            else:
                buff.write("{name}.dispense()\n".format(**tmplVars))

            buff.remove_indent(indented)

        if endRoutine is False:
            indented = self.writeStopTestCode(_buff)
            buff.remove_indent(indented)
            return

        buff.write(
            (
                "# test if {name} has finished dispensing\n"
                "if {name}.dispensed is not None:\n"
            ).format(**tmplVars)
        )
        with buff.indent():
            buff.write("continueRoutine = False # endRoutine\n")

    def writeRoutineEndCode(self, _buff):

        saveStats = self.params["saveStats"].val
        if saveStats is False:
            return

        if len(self.exp.flow._loopList):
            currLoop = self.exp.flow._loopList[-1]  # last (outer-most) loop
        else:
            currLoop = self.exp._expHandler

        tmplVars = {
            "name": self.params["name"],
            "loopName": currLoop.params["name"],
        }
        buff = _BufferContext(_buff)

        buff.write(
            (
                "# *{name}*: storing data\n"
                "{loopName}.addData('{name}.wanted',{name}.count)\n"
                "\n"
                "if isinstance({name}.dispensed,int):\n"
            ).format(**tmplVars)
        )
        with buff.indent():
            buff.write(
                # fmt: off
                (
                    "{loopName}.addData('{name}.dispensed',{name}.dispensed)\n"
                ).format(**tmplVars)
                # fmt: on
            )
        buff.write(
            "elif isinstance({name}.dispensed,PelletDispenserError):\n".format(
                **tmplVars
            )
        )
        with buff.indent():
            # fmt: off
            buff.write(
                (
                    "{loopName}.addData('{name}.dispensed',{name}.dispensed.dispensed)\n"
                    "logging.warn(f\"PelletDispenser '{name}' error: {{ {name}.dispensed }}\")\n"
                ).format(**tmplVars)
            )
            # fmt: on

        buff.write("else:\n")
        with buff.indent():
            buff.write(
                'logging.error("logic error in PelletDispenserComponent")\n'
            )

    def writeExperimentEndCode(self, _buff):
        buff = _BufferContext(_buff)
        tmplVars = {
            "name": self.params["name"],
        }
        # fmt: off
        buff.write((
            "# closing *{name}*\n"
            "{name}.close()\n"
        ).format(**tmplVars))
        # fmt: on
