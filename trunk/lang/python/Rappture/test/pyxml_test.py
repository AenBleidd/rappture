# PyXml Unit Tests

from Rappture import PyXml, RPENC_Z, RPENC_B64, RPENC_ZB64
from Rappture.encoding import encode, decode, isbinary
import numpy as np

rx = PyXml('test.xml')


def test_input1():
    global rx
    temp = rx['input.number(temperature).current'].value
    assert temp == '300K'


def test_input2():
    global rx
    temp = rx['input.number(temperature)']
    assert temp['current'].value == '300K'


def test_input2a():
    global rx
    temp = rx['input.number(temperature)']
    assert temp.get('current') == '300K'


def test_input2b():
    global rx
    temp = rx['input.number(temperature).current']
    assert temp.value == '300K'


def test_input3():
    global rx
    inp = rx['input']
    assert inp['number(temperature).current'].value == '300K'


def test_input4():
    global rx
    inp = rx['input']
    temp = inp['number(temperature)']
    assert temp['current'].value == '300K'


def test_input_old1():
    global rx
    temp = rx.get('input.number(temperature).current')
    assert temp == '300K'


def test_input_curve():
    global rx
    curve = rx['output.curve(f12).component.xy'].value
    curve = map(int, curve.split())
    assert curve == [0, 0, 1, 1, 2, 4, 3, 9, 4, 16]


def test_output_1():
    global rx
    curve = rx['output.curve(temp).component']
    curve['xy'] = "1 2"
    assert curve['xy'].value == "1 2"


def test_output_2():
    global rx
    curve = rx['output.curve(temp).component']
    curve['xy'] = [[1], [2]]
    assert curve['xy'].value == "1 2"


def test_output_3():
    global rx
    curve = rx['output.curve(temp).component']
    curve['xy'] = [[1.1], [2.2]]
    assert curve['xy'].value == "1.1 2.2"


def test_output_4():
    global rx
    curve = rx['output.curve(temp).component']
    curve['xy'] = ([1], [2])
    assert curve['xy'].value == "1 2"


def test_output_5():
    global rx
    curve = rx['output.curve(temp).component']
    curve['xy'] = [[1, 2, 3], [4, 5, 6]]
    assert curve['xy'].value == "1 4\n2 5\n3 6"


def test_output_6():
    global rx
    curve = rx['output.curve(temp).component']
    curve['xy'] = ([1, 2, 3], [4, 5, 6])
    assert curve['xy'].value == "1 4\n2 5\n3 6"


def test_output_7():
    global rx
    curve = rx['output.curve(temp).component']
    curve['xy'] = ((1, 2, 3), (4, 5, 6))
    assert curve['xy'].value == "1 4\n2 5\n3 6"


def test_output_8():
    global rx
    curve = rx['output.curve(temp).component']
    curve['xy'] = np.row_stack([[1, 2, 3], [4, 5, 6]])
    assert curve['xy'].value == "1 4 2 5 3 6"


def test_output_9():
    global rx
    rx.put('output.string(temp).current', 'xyzzy')
    assert rx['output.string(temp).current'].value == "xyzzy"


def test_output_10():
    global rx
    data = rx['output.string(temp)']
    data.put('current', 'xyzzy')
    assert rx['output.string(temp).current'].value == "xyzzy"


def test_output_11():
    global rx
    data = rx['output.string(temp)']
    data.put('current', 'xyzzy')
    assert data['current'].value == "xyzzy"


def test_output_12():
    global rx
    data = rx['output.string(temp)']
    data.put('current', 'xyzzy', compress=True)
    assert data['current'].value == "xyzzy"


def test_output_13():
    global rx
    data = rx['output.string(temp)']
    data.put('current', 'results.txt', type='file')
    assert data['current'].value == "These are the results of our test.\n"


def test_output_14():
    global rx
    data = rx['output.string(temp)']
    data.put('current', 'results.txt', type='file', compress=True)
    assert data['current'].value == "These are the results of our test.\n"


def test_encode_decode_Z():
    instr = "foobar\n012\0"
    x = encode(instr, RPENC_Z)
    outstr = decode(x, 0)
    assert instr == outstr


def test_encode_decode_B64():
    instr = "foobar\n012\0"
    x = encode(instr, RPENC_B64)
    outstr = decode(x, 0)
    assert instr == outstr


def test_encode_decode_ZB64():
    instr = "foobar\n012\0"
    x = encode(instr, RPENC_ZB64)
    outstr = decode(x, 0)
    assert instr == outstr


def test_isbinary_1():
    instr = "foobar\n012\0"
    assert isbinary(instr) == True


def test_isbinary_2():
    instr = "foobar\n"
    assert isbinary(instr) == False


def test_name0():
    global rx
    path = ''
    data = rx[path]
    assert data.name == path


def test_name1():
    global rx
    path = ''
    data = rx
    assert data.name == path


def test_name2():
    global rx
    path = 'output.string(temp)'
    data = rx[path]
    assert data.name == path


def test_name3():
    global rx
    path = 'output.string(temp)'
    out = rx['output']
    assert out['string(temp)'].name == path

