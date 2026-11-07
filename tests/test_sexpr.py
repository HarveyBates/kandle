from kandle_core import sexpr
from kandle_core.sexpr import Atom, SExpr


def test_parse_simple():
    node = sexpr.parse('(foo bar "baz qux")')
    assert node.head == "foo"
    assert isinstance(node.children[1], Atom)
    assert node.children[1].value == "bar"
    assert node.children[2].value == "baz qux"
    assert node.children[2].quoted is True


def test_nested_find():
    node = sexpr.parse('(lib (name "a") (type "KiCad") (name "b"))')
    assert [n.children[1].value for n in node.find_all("name")] == ["a", "b"]
    assert node.find("type").children[1].value == "KiCad"
    assert node.find("missing") is None


def test_roundtrip_structure_preserved():
    text = '(kicad_symbol_lib (version 20211014) (symbol "X" (y 1)))'
    node = sexpr.parse(text)
    reparsed = sexpr.parse(sexpr.dumps(node))
    assert reparsed.head == "kicad_symbol_lib"
    assert reparsed.find("symbol").children[1].value == "X"


def test_quoting_of_special_characters():
    node = SExpr([Atom("uri"), Atom("a b:c", quoted=False)])
    out = sexpr.dumps(node)
    # The space forces quoting even though quoted=False.
    assert '"a b:c"' in out


def test_escape_quotes_in_string():
    node = sexpr.parse(r'(a "he said \"hi\"")')
    assert node.children[1].value == 'he said "hi"'
    out = sexpr.dumps(node)
    assert r'\"hi\"' in out


def test_empty_string_is_quoted():
    node = SExpr([Atom("options"), Atom("", quoted=True)])
    assert '(options "")' in sexpr.dumps(node)


def test_unbalanced_raises():
    import pytest
    with pytest.raises(ValueError):
        sexpr.parse("(a (b)")
