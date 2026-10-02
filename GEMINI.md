# Micro Tank Arena (ESP32-C3 Laser Tag) — Development Guidelines

Derived from [Andrej Karpathy's observations](https://x.com/karpathy/status/2015883857489522876) on LLM coding pitfalls, adapted from `multica-ai/andrej-karpathy-skills`.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

---

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them — don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.
- **Embedded Specifics:** Verify ESP32-C3 pin limitations, ADC/PWM conflicts, RMT peripheral channels, and power/current constraints before writing hardware code.

---

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.
- **The test:** Would a senior embedded engineer say this is overcomplicated? If yes, simplify.

---

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it — don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

**The test:** Every changed line should trace directly to the user's request.

---

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add motor ramp" → "Write test for slew rate limiting, compile, verify PWM output slope"
- "Add IR protocol" → "Unit test encode & decode with bit checksum matching, verify roundtrip"
- "Fix WebSocket latency" → "Benchmark message dispatch frequency, ensure <50ms processing"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

---

## 5. Anti-Dead-Loop Protocol & Embedded Guardrails

1. **No Blind Retries:** Never repeat a tool call or compile command if it fails twice with the same error. Stop, analyze root cause, and change approach.
2. **Brownout Prevention:** Always enforce motor slew-rate limits to protect against ESP32 brownout during stall/reversal.
3. **Fail-Safe Mechanism:** Motor outputs must automatically stop if no valid WebSocket packet is received within 300ms.
4. **Channel Locking:** When in SoftAP + ESP-NOW hybrid mode, ensure Wi-Fi channel is fixed (Channel 1) across all nodes.
