# Inheritance Example

Demonstrates Spry's class/inheritance system using `class()` and `Object`.

## Features Demonstrated

| # | Feature | API |
|---|---------|-----|
| 1 | Defining a class | `class("ClassName")` |
| 2 | Inheritance | `class("Child", Parent)` |
| 3 | Method overrides | Overriding `update()` and `draw()` |
| 4 | `super()` calls | Calling parent methods from child |

## Classes

- **Animal**: Base class with `say()` method
- **Dog**: Inherits from Animal, overrides `say()`
- **Crab**: Inherits from Animal, overrides `say()`
