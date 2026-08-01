# StrixVerse UI Style Guide — Implementation Plan (updated: UI Completeness Pass)

## Context

StrixVerse is a custom 2D sandbox MMO. The brief asks for a complete, original UI style guide delivered as an interactive React showcase. The aesthetic is **Crystal Technology + Fantasy**: ancient magical crystals fused with modern tech, pixel art aesthetic, semi-transparent panels, crystal glow effects, deep blue/cyan/purple on dark slate backgrounds. The `create_make_theme` suggestions (warm, editorial, memphis) do not apply — the brief explicitly names the aesthetic, which takes precedence.

The deliverable is a single-page interactive style guide that demonstrates every UI component, laid out as a tabbed navigator.

---

## Design Decisions

### Fonts
- **Display / Headers:** `Press Start 2P` (Google Fonts) — authentic pixel font, perfect for headings and game titles
- **Body / UI Labels:** `VT323` (Google Fonts) — readable pixel font for body text, tooltips, chat
- **Mono / Data:** `Share Tech Mono` — for numbers, stats, coordinates, ping/fps counters

### Color Tokens (overriding theme.css)
```
--background: #1E2230        (deep slate game background)
--foreground: #FFFFFF
--card: #2C3145              (panel background)
--card-foreground: #FFFFFF
--primary: #4F8CFF           (primary blue)
--primary-foreground: #FFFFFF
--secondary: #6C5CE7         (crystal purple)
--secondary-foreground: #FFFFFF
--muted: #3A4060             (subdued panel)
--muted-foreground: #C7D0E0  (secondary text)
--accent: #4DE1FF            (crystal cyan highlight)
--accent-foreground: #1E2230
--border: rgba(106,127,181,0.5)  (#6A7FB5 at 50%)
--ring: #4DE1FF
--radius: 0.5rem
```

Additional CSS vars (defined in App.tsx inline style or globals):
- `--crystal-glow: #4DE1FF`
- `--success: #4CD964`
- `--warning: #FFD54A`
- `--danger: #FF5A5A`
- `--gold: #FFD700`

---

## Architecture

**Single file:** `src/app/App.tsx`

The app is a tabbed style guide with these top-level sections, navigated by a pixel-art tab bar:

1. **Overview** — Color palette swatches, typography scale, spacing system
2. **Buttons** — All button variants (idle, hover, pressed, disabled) + icon buttons
3. **HUD** — Full gameplay HUD mockup (top-left portrait/stats, top-right minimap, bottom hotbar, chat, quest tracker)
4. **Windows** — Inventory window, Equipment window, Crafting window, NPC Dialogue — tabbed within this section
5. **Icons** — Full icon pack grid (15 icons via SVG pixel-art paths or emoji-free lucide substitutes with pixel styling)
6. **Item Slots** — All slot states: empty, hover, selected, locked, rare, epic, legendary + stack numbers + durability
7. **Notifications** — All notification types displayed as a live stack
8. **Screens** — Splash, Login, Character Selection mockups as scaled-down previews

### Key Components (all in App.tsx)

```
<App>
  <GlobalBackground />         pixel art tiling bg via CSS pattern
  <TabNavigator />             top nav bar with crystal styling
  
  <OverviewTab />              palette + type scale + spacing
  <ButtonsTab />               variant matrix
  <HUDTab />                   full HUD mockup in a game-viewport frame
  <WindowsTab />               sub-tabbed: Inventory | Equipment | Crafting | NPC Dialogue
  <IconsTab />                 icon grid with labels
  <ItemSlotsTab />             slot states grid
  <NotificationsTab />         live notification showcase with trigger buttons
  <ScreensTab />               login / splash / char-select previews
```

---

## Visual Craft Details

### Crystal Panel
- `bg-[#2C3145]` base
- `border border-[#6A7FB5]/50`
- `rounded-lg`
- Subtle inner highlight: `box-shadow: inset 0 1px 0 rgba(77,225,255,0.15), 0 4px 24px rgba(0,0,0,0.4)`
- Drop shadow: `shadow-[0_8px_32px_rgba(0,0,0,0.5)]`

### Crystal Button
- Idle: blue border + soft inner glow
- Hover: brightness up + cyan `box-shadow: 0 0 12px rgba(77,225,255,0.5)`
- Active: `translateY(1px)` + glow reduces
- Disabled: `opacity-40 grayscale`

### HUD
- All 4 corners anchored, compact
- Health bar: green fill with pixel-segmented appearance
- Energy bar: cyan fill
- XP bar: purple fill
- Minimap: rounded square with dot markers
- Hotbar: 10 slots centered at bottom

### Inventory Window
- Draggable header (visual only — no actual drag in static showcase, but pointer cursor)
- 5×6 grid of item slots
- Tabs: All | Equipment | Consumables | Materials
- Stats sidebar

### Pixel Art Background
CSS `background-image` using a repeating dot grid pattern:
```css
background-image: radial-gradient(circle, #3A4060 1px, transparent 1px);
background-size: 24px 24px;
```

### Animations (via Tailwind + CSS)
- `@keyframes crystalPulse` — subtle glow pulse on accent elements
- `@keyframes fadeInUp` — notification entry
- `@keyframes scanline` — subtle scanline sweep on HUD elements
- Button hover transitions: `transition-all duration-150`
- Window open: `animate-[fadeInScale_0.2s_ease-out]`

---

## Files to Modify

| File | Change |
|------|--------|
| `src/styles/fonts.css` | Add Google Fonts imports for Press Start 2P, VT323, Share Tech Mono |
| `src/styles/theme.css` | Update `:root` tokens to StrixVerse palette (dark mode only — both `:root` and `.dark` set to same values since this is always dark) |
| `src/app/App.tsx` | Full implementation — all components, tabs, state, animations |

---

## Economy Phase — Additions

### Scope
All changes are in `src/app/App.tsx`. No new files needed.

### 1 — Expand `WinTab` type
Add `"wallet"` and `"shop"` to the union. Add both to the `SUB` array inside `WindowsTab`, placing **Wallet** after Quest Log and **Shop** before NPC.

### 2 — `WalletSubTab` content
A two-column panel:

**Left column — Currency summary**
- 🪙 Coins row: large balance (`52,450`), "Standard gameplay currency", coin-earn sources listed (Farming, Mining, Quests, Trading…), coin-spend uses (NPC shop, World Locks, Crafting…)
- 💎 Gems row: live balance (`gems` state), "Premium currency", obtain sources (Purchase, Events, Giveaways), note: *"Never farmable through gameplay"*
- Separator: "World Lock" info card — price 50,000 🪙 Coins (never Gems)

**Right column — Transaction history table**
- Static mock rows: date, description, amount (+/-), currency type (coin or gem)
- Example rows: Quest Reward +500🪙, Marketplace Sale +1200🪙, Gem Pack Purchase +1000💎, Cosmetic Purchase -300💎, NPC Purchase -250🪙
- "Coins and Gems are tracked in separate ledgers" notice

**Bottom — Gem Pack purchase UI**
- Four pack cards: Starter (100💎 · $0.99), Popular (550💎 · $4.99 · BEST VALUE badge), Value (1200💎 · $9.99), Premium (2800💎 · $19.99)
- Each card has a "BUY" button (disabled in the showcase with a tooltip)
- Small print: "Gems are a premium currency. All purchases are final and non-refundable."

### 3 — `PremiumShopSubTab` content
Full-featured shop panel:

**Header bar**
- "PREMIUM SHOP" title with gem icon
- Player gem balance displayed
- "No Pay-to-Win • Cosmetics Only" badge in green

**Category nav** (horizontal pills)
Featured | Cosmetics | Hats | Wings | Auras | Pets | Emotes | Titles | Furniture | Seasonal | Bundles

**Featured banner** (top)
- Large card: "Crystal Founder's Pack" — limited-time bundle, 1200💎, lists contents (Crystal Wings + Crown Hat + Arcane Aura + Title "Founder")

**Item grid** — 12 shop cards, each showing:
- Item icon (emoji)
- Item name
- Category badge
- Gem price with icon
- "BUY" button (cyan, disabled in showcase)
- "LIMITED" or "NEW" badge where applicable

Sample items (cosmetics only, no stat advantages):
| Name | Category | Price |
|------|----------|-------|
| Crystal Wings | Wings | 300💎 |
| Shadow Hood | Hats | 150💎 |
| Arcane Aura | Auras | 250💎 |
| Mini Drake Pet | Pets | 400💎 |
| Victory Dance | Emotes | 80💎 |
| "Crystal Lord" | Titles | 120💎 |
| Crystal Throne | Furniture | 180💎 |
| Name Change | Service | 200💎 |
| Inventory +10 | Expansion | 500💎 |
| Wardrobe +5 | Expansion | 300💎 |
| Spring Bundle | Seasonal | 800💎 |
| Crystal Block Set | Blocks | 220💎 |

**Notice panel** at bottom
- "The Premium Shop never sells weapons, armor, stat boosts, or gameplay advantages."
- "Gems can only be obtained through official channels — never through farming."

### 4 — HUD click interactions
The HUD currently shows Coins and Gems with `title` attributes but no real click action. Add an `onOpenWindow` callback prop pattern: lift `windowsSubTab` state up to `App`, pass a setter into `HUDTab` so clicking Coins sets `tab → "windows"` + `subTab → "wallet"`, clicking Gems sets `tab → "windows"` + `subTab → "shop"`. This requires:
- Lifting `tab` state up (already in `App`)
- Adding `onCoinsClick` / `onGemsClick` props to `HUDTab`
- Lifting `subTab` state out of `WindowsTab` into `App` and threading it down

### 5 — Coins state and animation
Currently coins are hardcoded as `4280`. Add `const [coins, setCoins] = useState(52450)` and a `coinAnim` state mirroring `gemAnim`. Thread coins value into both the HUD display and the Wallet panel.

### 6 — Marketplace clarification banner
Add a small info banner inside the existing Marketplace sub-tab confirming "Marketplace uses 🪙 Coins only. Gems are never accepted here."

### Files modified
- `src/app/App.tsx` — all changes

### Verification
- Open Windows tab → Wallet: both currency summaries visible, transaction history, gem pack cards
- Open Windows tab → Shop: category nav works, items display with gem prices, no stat-boosting items
- HUD: click Coins → navigates to Windows → Wallet; click Gems → navigates to Windows → Shop
- HUD balance shows 52,450 Coins and live gem balance
- Marketplace banner confirms Coins-only

---

## Implementation Order

1. Update `fonts.css` — Google Fonts imports
2. Update `theme.css` — StrixVerse tokens
3. Write `App.tsx`:
   a. Global styles + keyframes (via `<style>` tag injected)
   b. Design tokens and utility components (CrystalPanel, CrystalButton, PixelIcon, ProgressBar, ItemSlot)
   c. Tab navigation
   d. OverviewTab (palette + type)
   e. ButtonsTab
   f. HUDTab (the showpiece)
   g. WindowsTab (Inventory as primary, others as stubs)
   h. IconsTab
   i. ItemSlotsTab
   j. NotificationsTab (with useState for live demo)
   k. ScreensTab (Login mockup)

---

## Verification

- Open the app and confirm the tab navigator works
- Verify HUD mockup renders all 4 corners correctly
- Verify inventory grid renders slot states
- Verify button hover/active states respond
- Verify notifications can be triggered and fade in
- Confirm pixel fonts load (check for font rendering in browser)
- Check at 1280px and 1920px viewport widths

---

## UI Completeness Pass — New Phase

### Context

A full brief audit (`strixverse-ui-design.md`) identified 6 features called for in the spec that are not yet implemented. Everything else already exists. This phase adds only the missing pieces without touching existing sections.

All changes are in `src/app/App.tsx` only.

---

### What Already Exists (do not rebuild)

All 8 main tabs, all 14 Windows sub-tabs, all 11 Screens, reusable components (CrystalPanel, Btn, Bar, LabelBar, CurrencyDisplay, SvgIcon), GLOBAL_CSS keyframes, and CSS classes are complete. The shop (unified coin + gem), wallet, HUD, world selection, and connecting flow are all done.

---

### 6 Additions Required

#### 1 — UITooltip Component

**What:** A `UITooltip` component that appears on item hover in Inventory slots and Shop item cards. Shows: item name (rarity-colored), description, stat rows, durability bar, requirements.

**New types** (add before `WindowsTab`):
```ts
type TooltipItem = {
  name: string; description: string;
  rarity: "common" | "rare" | "epic" | "legendary";
  stats?: { label: string; value: string; color?: string }[];
  durability?: { current: number; max: number };
  requirements?: string[];
};
```

**Component** — uses `position: fixed`, `z-index: 9999`, `pointer-events: none`, `animation: fadeInScale .12s`. Add `.sv-tooltip { min-width:200px; max-width:280px; pointer-events:none; z-index:9999; }` to GLOBAL_CSS.

**State** — add to top of `WindowsTab`:
```ts
const [tooltip, setTooltip] = useState<TooltipItem | null>(null);
const [tooltipPos, setTooltipPos] = useState({ x: 0, y: 0 });
```

**Trigger** — `onMouseEnter` / `onMouseMove` / `onMouseLeave` on each named inventory slot and shop item card.

**Render** — at bottom of `WindowsTab` return, before closing `</div>`:
```tsx
{tooltip && <UITooltip item={tooltip} x={tooltipPos.x} y={tooltipPos.y}/>}
```

**INV_ITEMS** — extend the 12 named items with `desc` and `stats` optional fields. Null slots unchanged.

Rarity color map: `common → #C7D0E0`, `rare → #4F8CFF`, `epic → #9B59B6`, `legendary → #FFD700`.

**Insert locations:** `UITooltip` function after `Btn` (~line 422). State vars and render point inside `WindowsTab`.

---

#### 2 — Settings Windows Sub-tab

**What:** A Settings window inside the Windows tab (alongside Inventory, Shop, etc.) with 6 category tabs: GRAPHICS, AUDIO, CONTROLS, GAMEPLAY, ACCESSIBILITY, UI SCALE.

**Type change** — extend `WinTab` union with `"settings"`.

**SUB array** — add `{ id: "settings", label: "⚙ SETTINGS", accent: "#C7D0E0" }` after `npc`.

**New state** in `WindowsTab` top block:
```ts
const [settingsTab, setSettingsTab] = useState("GRAPHICS");
```

**Panel structure** (inside `{subTab === "settings" && ...}`):
- `CrystalPanel` wrapping a `PanelHeader` + horizontal layout
- Left sidebar (160px): 6 category buttons with active highlight (blue left border + background tint), pattern identical to existing ScreensTab Settings sidebar
- Right content (scrollable): each category shows 6-8 setting rows using the same row pattern as ScreensTab Settings (`border: "1px solid rgba(106,127,181,0.16)"`, vtFont label, toggle/select/value controls on right)
  - GRAPHICS: Resolution, Render Quality, Shadow Quality, Particle Effects, VSync
  - AUDIO: Master Volume, Music Volume, SFX Volume (rendered as mini bar + value)
  - CONTROLS: Key binding rows (action | key badge)
  - GAMEPLAY: Autoloot, PvP, Tutorial Hints, Damage Numbers (toggles)
  - ACCESSIBILITY: Colorblind Mode, Large Text, Reduce Motion, Screen Reader
  - UI SCALE: Scale slider preview (75% / 100% / 125% / 150% options)
- Footer: SAVE CHANGES + RESET TO DEFAULTS buttons

**No new CSS needed** — reuses existing row and toggle patterns.

---

#### 3 — Inventory Search + Sort

**What:** Search input and sort buttons above the inventory grid.

**New state** in `WindowsTab`:
```ts
const [invSearch, setInvSearch] = useState("");
const [invSort, setInvSort] = useState<"default"|"name"|"rarity"|"qty">("default");
```

**Constant** near `INV_ITEMS`: `const RARITY_ORDER = { common:0, rare:1, epic:2, legendary:3 };`

**UI** — inside the existing Inventory filter row (before the grid), add to the right side:
- `<input className="sv-input">` placeholder "Search items..."
- 4 sort buttons: DEFAULT / NAME / RARITY / QTY (active button gets `color: #4DE1FF, borderColor: rgba(77,225,255,0.40)`)

**Grid** — derive `displayedItems` inline (no `useMemo` needed):
```ts
let displayedItems = INV_ITEMS.filter(it => it && (!invSearch || it.name.toLowerCase().includes(invSearch.toLowerCase())));
// apply sort, then pad to 30 slots with nulls
```

Replace `INV_ITEMS.map(...)` with `displayedItems.map(...)`.

---

#### 4 — Storage Transfer Buttons, Sort, Search

**What:** Search + sort + "TRANSFER →" button in the Storage sub-tab header.

**New state** in `WindowsTab`:
```ts
const [storageSearch, setStorageSearch] = useState("");
const [storageSort, setStorageSort] = useState<"default"|"name">("default");
```

**UI** — replace the single-row header above the storage grid with a two-row header:
- Row 1: chest name label + DEPOSIT ALL + WITHDRAW + **TRANSFER →** (variant="success")
- Row 2: search input (flex: 1) + DEFAULT / NAME sort buttons

Grid items filtered by `storageSearch` inline.

---

#### 5 — Loading Screen Tips

**What:** Replace the static flavor-text quote at the bottom of the Loading screen with a styled "💡 LOADING TIP" box showing a random tip.

**Data** — constant before `ScreensTab`:
```ts
const LOADING_TIPS = [
  "World Locks protect your build. Always lock your world before going offline!",
  "Plant Crystal Seeds near water tiles for faster growth.",
  "Check the Marketplace daily — rare items often appear at discount.",
  "Completing quests grants bonus XP, Coins, and crafting recipes.",
  "Gems can only be obtained via official packs or seasonal events.",
  "Join a Guild to unlock co-op dungeon access and guild rewards.",
];
```

**State** — add to `ScreensTab` top:
```ts
const [tipIdx] = useState(() => Math.floor(Math.random() * LOADING_TIPS.length));
```

**UI** — replace the existing bottom `<div>` (the italic quote) with:
```tsx
<div style={{ position:"absolute", bottom:14, left:"50%", transform:"translateX(-50%)",
  width:420, padding:"10px 16px", borderRadius:8,
  background:"rgba(14,18,30,0.75)", border:"1px solid rgba(77,225,255,0.20)",
  backdropFilter:"blur(6px)", textAlign:"center" }}>
  <span style={{ ...pxFont, fontSize:5, color:"#4DE1FF", display:"block", marginBottom:4 }}>💡 LOADING TIP</span>
  <span style={{ ...vtFont, fontSize:15, color:"#C7D0E0", lineHeight:1.5 }}>{LOADING_TIPS[tipIdx]}</span>
</div>
```

---

#### 6 — Wallet Statistics Panel

**What:** A full-width stats strip below the two-column flex row in the Wallet sub-tab, before the Gem Pack purchase section.

**UI** — insert after the closing `</div>` of the coin/gem summaries + transaction history row:
```tsx
<div style={{ margin:"0 16px", paddingTop:14, borderTop:"1px solid rgba(106,127,181,0.18)" }}>
  <div style={{ ...pxFont, fontSize:7, color:"#4DE1FF", marginBottom:12 }}>STATISTICS</div>
  {/* 6 stat cards in a flex-wrap row */}
  {/* Total Coins Earned, Total Coins Spent, Total Gems Purchased, Total Gems Spent, Marketplace Trades, Largest Transaction */}
  {/* Coin/Gem balance ratio bar below cards */}
</div>
```

Stat cards: `flex: "1 1 160px"`, `border: "1px solid rgba(106,127,181,0.20)"`, label in `pxFont` size 5, value in `monoFont` size 16 with currency-appropriate color.

Ratio bar: two-segment div (97.6% gold / 2.4% cyan) showing Coins vs Gems total balance split.

---

### Files Modified

| File | Change |
|------|--------|
| `src/app/App.tsx` | All 6 additions — types, state, components, JSX |

---

### Implementation Order

1. Add `.sv-tooltip` to GLOBAL_CSS + `TooltipItem` type + `UITooltip` component (after `Btn`)
2. Extend `WinTab` type with `"settings"`, add `settingsTab` state to `WindowsTab`
3. Add Settings panel JSX block (after the Shop IIFE block)
4. Add `invSearch`, `invSort`, `storageSearch`, `storageSort`, `tooltip`, `tooltipPos` state to `WindowsTab`
5. Wire inventory search + sort + tooltip triggers into Inventory sub-tab
6. Wire storage search + sort + transfer into Storage sub-tab
7. Wire tooltip trigger onto Shop item cards
8. Add `LOADING_TIPS` + `tipIdx` state, replace quote in Loading screen
9. Add Wallet statistics section

Steps 5–9 are independent of each other once step 4 is done.

---

### Verification

- Windows → Shop: hover an item card → tooltip appears at cursor position with name, description, price, and rarity badge
- Windows → Inventory: hover a named item slot → tooltip shows stats and durability bar; hover an empty slot → no tooltip
- Windows → Settings: tab appears in sub-tab bar; clicking each of 6 categories updates the right panel content
- Windows → Inventory: type in search box → grid filters live; click sort buttons → order changes
- Windows → Storage: search box filters grid; TRANSFER → button visible; sort buttons present
- Screens → Loading: tip box visible at bottom with "💡 LOADING TIP" label and tip text
- Windows → Wallet: Statistics section visible below transaction history with 6 stat cards and ratio bar
