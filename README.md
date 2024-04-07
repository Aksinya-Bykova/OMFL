OneMoreFunnyLanguage format

OMFL is a format for storing configuration files. It is similar in nature to ini, but is more flexible. Its main purpose is the ability to obtain a value from a config for a specific key and logically divide it into sections and subsections.

## Task

Your task is to implement a parser for OMFL

## Format

An OMFL file is an ASCII encoded text file in the OMFL format. The format specification will be described next. The basic construct for OMFL is the KeyValue pair.
The format is case sensitive. Spaces and empty lines are ignored.

### Key\Value

The key is on the left, then the equals sign, then the value.
```text
key = "value"
```

Both key and value are required. Line break is prohibited.
The value cannot be overridden for the same key (within the same section)

#### Key

The key may consist of:

- Uppercase and lowercase Latin letters
- Digits
- Characters '-' and '_'

The key cannot have zero length.

```text
number = 0
name = "M3100"
```

#### Meaning

The value can be one of the following types

- integer
- real number
- line
- logical variable
- array of values

#### Integer

Consists of numbers (one or more). It is possible to add a '+' or '-' character as the first character.

```text
key1 = 1
key2 = -2022
key3 = +128
```

Possible value will fit in int32_t

#### Real number

Consists of numbers (one or more) and one '.'. There must be at least one number before and after the period
It is possible to add a '+' or '-' character as the first character.

```text
key1 = 1.23
key2 = -2.77
key3 = -0.0001
```

##### Line

The string is surrounded by double quotes. Contains any characters.

```text
key1 = "value"
key2 = "Hello world!"
```

#### Boolean value

For boolean values the literal "true" or "false" is used

```text
key1 = true
key2 = false
```

#### Array

The array is surrounded by '[' and ']' characters. Elements are separated by ','.
The array can consist of any valid Values, not necessarily of the same type

```text
key1 = [1, 2, 3, 4, 5]
key2 = ["Hello", "world"]
key3 = [[1, 2, 3, 4, 5], ["Hello", "world"]]
key4 = [1, 3.3, "ITMO", [true, false]]
```

### Sections

In addition to the key-value block, the format supports sections. Sections allow you to combine Key\Value sets into logical blocks.
A section is defined as a Key surrounded by '[' and ']'

```text
[name]
```

After a section is declared, all subsequent Key\Value pairs belong to this section until the next one is declared

```text
[section-1]
key1 = 1
key2 = "2"

[section-2]
key1 = 2
```

Although the section is subject to the melting keys, it may also contain a '.' symbol. What defines nested sections.

```text
[section-1.part-1.x]

[section-1.part-1.y]

[section-1.part-2.x]

[section-2.z]
```

Thus, a section can contain both Key\Value pairs and other sections. The key and the subsection name cannot match

```text
[A]
key1 = 1

[A.B]
key2 = 3

[A.B.C]
key3 = 3
```

#### Comments

The config may contain a one-line comment. Comments begin with a '#' character, unless the '#' is inside a string value.

```text
     key1 = "value1" # some comment
     # another comment
```

## Parser

The goal of the work is to implement a parser in the OMFL format.

The parser must

- Validate the correctness of the file according to the OMFL Format
- Read data from a file into an object whose class has an interface that allows you to get a section or value by key.
- What interface the parser should have is described in the tests

Because We are not yet familiar with exceptions, then when we try to get a value or section using a non-existent key, the returned value is undefined.

## Tests

The tests directory contains only basic format tests and a parser.
It is recommended to write tests for the data structures and functions that you create and possibly supplement existing ones.
