# Visible widget ownership,2026-11-04

The production metadata maps up to6144 retained GPU cache ids onto at most192
native widget owners. It allocates no heap objects and calls no UI/native API.
The renderer must hide/clear old widget textures before Unbind/reuse and reset
the metadata when its widget layer is destroyed. Retained GPU entries continue
to exist independently of visible widget bindings.

`run.py` compiles the actual header with VC10/Od and/O2. Tests visit all6144
distinct physical cache ids while exactly192 visible bindings are maintained,
exercise full-pool refusal without mutation, repeated binds, last-cache-slot
reuse, invalid/overflow ids and full repeated reset. Find/Unbind are constant
time; Bind scans at most192 metadata entries only on a new visible assignment.
This proves pool ownership, not actual MyGUI draw performance or cleanup.
