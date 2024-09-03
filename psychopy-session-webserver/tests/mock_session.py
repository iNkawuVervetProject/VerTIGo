import threading
import time
from pathlib import Path
from unittest.mock import Mock


def build_mock_session(root):
    root = Path(root).resolve()
    mock = Mock()

    fooExp = Mock()
    barExp = Mock()

    mock.root = str(root)
    mock.win = None

    mock.currentExperiment = None
    mock._currentExperiment = None

    mock.experiments = {
        "foo.psyexp": None,
        "bar.psyexp": None,
    }
    mock.experimentObjects = {
        "foo.psyexp": fooExp,
        "bar.psyexp": barExp,
    }

    fooExp.getResourceFiles.return_value = [
        {"rel": "foo.png", "abs": str(root.joinpath("foo.png")), "name": "foo.png"},
    ]

    barExp.getResourceFiles.return_value = [
        {
            "rel": "baz/bar.png",
            "abs": str(root.joinpath("baz/bar.png")),
            "name": "bar.png",
        },
        {
            "rel": "../absolute.png",
            "abs": str(root.joinpath("../absolute.png").resolve()),
            "name": "asbolute.png",
        },
    ]

    expInfos = {
        "foo.psyexp": {
            "participant": 123,
            "session": 1,
            "expName|hid": "foo",
            "date|hid": "foo",
            "psychopy_version|hid": "1.1.1",
        },
        "bar.psyexp": {
            "participant": 123,
            "expName|hid": "bar",
            "session": 1,
            "rewards": 3,
            "date|hid": "foo",
            "psychopy_version|hid": "1.1.1",
        },
    }

    def getExpInfos(key, sessionParams=False):
        return expInfos[key]

    mock.getExpInfoFromExperiment.side_effect = getExpInfos

    def openWindow(*args, **kwargs):
        mock.win = Mock()
        mock.win.getActualFrameRate.return_value = 30.0

    mock.setupWindowFromExperiment.side_effect = openWindow

    def runExperiment(key, params, blocking):
        mock._currentExperiment = key
        time.sleep(0.1)
        mock._currentExperiment = None

    mock.runExperiment.side_effect = runExperiment

    return mock
