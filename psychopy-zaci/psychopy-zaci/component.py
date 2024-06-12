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
            inputType="single",
            updates="constant",
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
                "Should the dispensing action force the end of the Routine"
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
        code = """from psychopy_zaci import PelletDispenserDevice, PelletDispenserError

        %(name)s = PelletDispenserDevice(win)

        """
        buff.writeIndentedLines(code % self.params)

    def writeFrameCode(self, buff):
        buff = _BufferContext(buff)
        endRoutine = self.params["endRoutineOnDispense"].val

        buff.write("# *%s* updates\n" % self.params["name"].val)

        indented = self.writeStartTestCode(buff)
        if indented:
            code = """%(name)s.dispense(%(count)d)
            """
            buff.write(code % self.params)
            buff.remove_indent(indented)

        indented = self.writeStopTestCode(buff)
        buff.remove_indent(indented)

        if endRoutine is False:
            return

        buff.write("""# test if %(name)s has finished dispensing
        if %(name).dispensed is not None:
        """ % self.params)
        with buff.indent():
            buff.write("""continueRoutine = False # endRoutine
            """)

    def writeRoutineEndCode(self, buff):
        saveStats = self.params["saveStats"].val
        if saveStats is False:
            return
        name = self.params["name"]

        buff = _BufferContext(buff)
        if len(self.exp.flow._loopList):
            currLoop = self.exp.flow._loopList[-1]  # last (outer-most) loop
        else:
            currLoop = self.exp._expHandler

        buff.write("""# store data for {name}
        if isinstance({name}.dispensed,int):
        """.format(name=name))
        with buff.indent():
            buff.write(
                """{loopName}.addData('{name}.dispensed', {name}.dispensed)
            """.format(loopName=currLoop.params["name"], name=name)
            )
        buff.write("""elif isinstance({name}.dispensed,PelletDispenserError):
        """.format(name=name))
        with buff.indent():
            buff.write(
                """{loopName}.addData('{name}.dispensed',{name}.dispensed.dispensed)
            #TODO: log error for dispensing the correct amount
            """.format(
                    loopName=currLoop.params["name"], name=name
                )
            )
        buff.write("""else:
        # TODO: log logic error in component
        """)
        with buff.indent():
            buff.write("""pass
            """)
