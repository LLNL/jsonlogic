## jsonlogic.py - JSON Logic expression generator

This package provides functionality to express JSON Logic using standard Python datastructures.

An example:

```python
>>> from jsonlogic import Variable
>>> v1 = Variable('var1')
>>> v2 = Variable('var2')
>>> e = (v1 < v2)
>>> print(e)
{"<": [{"var": "var1"}, {"var": "var2"}]}
>>> print (v1 < 3)
{"<": [{"var": "var1"}, 3]}
>>> print ( (v1 < 3) & (v1 > v2))
{"and": [{"<": [{"var": "var1"}, 3]}, {">": [{"var": "var1"}, {"var": "var2"}]}]}
>>> print ( (v1 < 3) & ~(v1 > v2)))  # ~ is "not"
{"and": [{"<": [{"var": "v1"}, 3]}, {"not": [{">": [{"var": "v1"}, {"var": "v2"}]}]}]}
```
