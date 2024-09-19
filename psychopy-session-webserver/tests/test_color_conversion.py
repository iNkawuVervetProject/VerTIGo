import pytest

from psychopy_session_webserver.utils import convertToPsychopy


@pytest.mark.parametrize(
    "input,expected",
    [
        ("#abc", (20 / 15 - 1, 22 / 15 - 1, 24 / 15 - 1)),
        ("#000000", (-1.0, -1.0, -1.0)),
        ("#ffffff", (1.0, 1.0, 1.0)),
        ("#7f7f7f", (0.0, 0.0, 0.0)),
    ],
)
def test_color_conversion(input, expected):
    res = convertToPsychopy(input)

    assert len(res) == 3
    for r, e in zip(res, expected):
        assert abs(r - e) < 1e-2
