# Snowbreak Background Audio

**解决卜卜互动场景中，Windows端切到后台就停止播放的问题**

卜卜打电话互动场景推出了白噪音功能，应当有一个后台播放功能。现有客户端并不支持。

**Fixes the issue where audio stops playing when Snowbreak is minimized on Windows.**

Bubu's phone call interaction scene has a white noise feature that should work in background. The current client doesn't support this.

---

## 使用方法 | Usage

1. 将 `Launcher.exe` 和 `WndProcHook.dll` 放在同一文件夹
2. 双击 `Launcher.exe`
3. 自动通过 Steam 启动游戏并注入
4. 现在可以切到其他窗口，游戏音频会继续播放

---

1. Place `Launcher.exe` and `WndProcHook.dll` in the same folder
2. Double-click `Launcher.exe`
3. Game will launch via Steam and inject automatically
4. You can now switch to other windows while audio keeps playing

---

## 原理 | How it works

```
原始流程（静音）:
WM_ACTIVATEAPP(FALSE) → WndProc → UE4 → Wwise暂停

Hook后流程（音频继续）:
WM_ACTIVATEAPP(FALSE) → HookedWndProc → 丢弃 → (游戏不知道发生了什么)
```

核心：不是欺骗游戏"你还在前台"，而是直接不让游戏收到"失焦"消息。

The tool intercepts Windows messages and blocks the "lost focus" notification from reaching the game.

---

## 风险提示 | Warning

> [!CAUTION]
> 本工具使用 DLL 注入修改游戏进程的窗口消息处理。
>
> **仅适用于 Steam 国际服**（无内核级反作弊）。
>
> **国服使用 ACE 反作弊，注入行为会被检测，可能导致封号。**

> [!CAUTION]
> This tool uses DLL injection to modify the game's window message handling.
>
> **Steam Global version ONLY** (no kernel-level anti-cheat).
>
> **CN version uses ACE anti-cheat. Injection will be detected and may result in ban.**

---

## License

MIT
