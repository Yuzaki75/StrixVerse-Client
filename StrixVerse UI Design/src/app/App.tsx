import React, { useState } from "react";

// ─── Global CSS injected at runtime ─────────────────────────────────────────
const GLOBAL_CSS = `
  @keyframes crystalPulse {
    0%,100% { box-shadow: 0 0 8px rgba(77,225,255,0.3), inset 0 0 8px rgba(77,225,255,0.05); }
    50%      { box-shadow: 0 0 20px rgba(77,225,255,0.7), inset 0 0 16px rgba(77,225,255,0.12); }
  }
  @keyframes fadeInUp {
    from { opacity: 0; transform: translateY(10px); }
    to   { opacity: 1; transform: translateY(0); }
  }
  @keyframes fadeInScale {
    from { opacity: 0; transform: scale(0.96); }
    to   { opacity: 1; transform: scale(1); }
  }
  @keyframes glowPulse {
    0%,100% { opacity: 0.55; }
    50%      { opacity: 1; }
  }
  @keyframes legendaryShimmer {
    0%   { border-color: #FFD700; box-shadow: 0 0 10px rgba(255,215,0,0.4); }
    50%  { border-color: #FFA500; box-shadow: 0 0 18px rgba(255,165,0,0.6); }
    100% { border-color: #FFD700; box-shadow: 0 0 10px rgba(255,215,0,0.4); }
  }
  @keyframes gemEarn {
    0%   { transform: scale(1);    box-shadow: 0 0 6px rgba(77,225,255,0.25); }
    30%  { transform: scale(1.14); box-shadow: 0 0 18px rgba(77,225,255,0.80); }
    70%  { transform: scale(1.07); box-shadow: 0 0 12px rgba(77,225,255,0.55); }
    100% { transform: scale(1);    box-shadow: 0 0 6px rgba(77,225,255,0.25); }
  }
  @keyframes gemSpend {
    0%,100% { transform: translateX(0); }
    15%     { transform: translateX(-3px); }
    35%     { transform: translateX(3px); }
    55%     { transform: translateX(-2px); }
    75%     { transform: translateX(2px); }
  }
  @keyframes gemFloat {
    0%   { opacity: 1; transform: translateY(0)   scale(1);    }
    70%  { opacity: 1; transform: translateY(-22px) scale(1.05); }
    100% { opacity: 0; transform: translateY(-36px) scale(0.9);  }
  }
  .sv-gem-earn  { animation: gemEarn  0.45s ease-out forwards; }
  .sv-gem-spend { animation: gemSpend 0.35s ease-out forwards; }
  .sv-gem-float {
    position: absolute;
    pointer-events: none;
    animation: gemFloat 1.1s ease-out forwards;
    font-family: 'Share Tech Mono', monospace;
    font-size: 10px;
    font-weight: 700;
    text-shadow: 0 1px 4px rgba(0,0,0,0.8);
    white-space: nowrap;
    z-index: 50;
  }

  /* ── Tooltip ── */
  .sv-tooltip {
    min-width: 200px;
    max-width: 280px;
    pointer-events: none;
    z-index: 9999;
    animation: fadeInScale .12s ease-out;
  }

  /* ── Pixel grid background ── */
  .sv-pixel-grid {
    background-image: radial-gradient(circle, rgba(58,64,96,0.55) 1px, transparent 1px);
    background-size: 24px 24px;
  }

  /* ── Crystal panel ── */
  .sv-panel {
    background: #2C3145;
    border: 1px solid rgba(106,127,181,0.45);
    border-radius: 10px;
    box-shadow: inset 0 1px 0 rgba(77,225,255,0.10), 0 8px 32px rgba(0,0,0,0.55);
  }
  .sv-panel-header {
    background: linear-gradient(180deg, rgba(79,140,255,0.10) 0%, transparent 100%);
    border-bottom: 1px solid rgba(106,127,181,0.28);
    padding: 11px 16px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    cursor: default;
  }
  .sv-panel-drag { cursor: grab; }

  /* ── Crystal button ── */
  .sv-btn {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    font-family: 'Press Start 2P', monospace;
    font-size: 9px;
    color: #fff;
    background: linear-gradient(180deg, #3a4d7a 0%, #2d3a5e 100%);
    border: 1px solid rgba(79,140,255,0.60);
    border-radius: 6px;
    padding: 9px 18px;
    cursor: pointer;
    user-select: none;
    transition: all 0.14s ease;
    box-shadow: 0 0 8px rgba(79,140,255,0.18), inset 0 1px 0 rgba(255,255,255,0.10);
    text-shadow: 0 1px 2px rgba(0,0,0,0.8);
  }
  .sv-btn:hover {
    background: linear-gradient(180deg, #4a5e8e 0%, #3c4b70 100%);
    border-color: rgba(77,225,255,0.80);
    box-shadow: 0 0 16px rgba(77,225,255,0.50), inset 0 1px 0 rgba(255,255,255,0.14);
  }
  .sv-btn:active { transform: translateY(1px); box-shadow: 0 0 6px rgba(77,225,255,0.20); }

  .sv-btn-purple {
    background: linear-gradient(180deg, #3d2f70 0%, #2d2258 100%);
    border-color: rgba(108,92,231,0.65);
    box-shadow: 0 0 8px rgba(108,92,231,0.22), inset 0 1px 0 rgba(255,255,255,0.10);
  }
  .sv-btn-purple:hover {
    border-color: rgba(108,92,231,0.95);
    box-shadow: 0 0 16px rgba(108,92,231,0.55);
  }
  .sv-btn-success {
    background: linear-gradient(180deg, #1e5c30 0%, #164424 100%);
    border-color: rgba(76,217,100,0.65);
    box-shadow: 0 0 8px rgba(76,217,100,0.22), inset 0 1px 0 rgba(255,255,255,0.10);
  }
  .sv-btn-success:hover { border-color: rgba(76,217,100,0.95); box-shadow: 0 0 16px rgba(76,217,100,0.55); }
  .sv-btn-danger {
    background: linear-gradient(180deg, #6e2020 0%, #4e1616 100%);
    border-color: rgba(255,90,90,0.65);
    box-shadow: 0 0 8px rgba(255,90,90,0.22), inset 0 1px 0 rgba(255,255,255,0.10);
  }
  .sv-btn-danger:hover { border-color: rgba(255,90,90,0.95); box-shadow: 0 0 16px rgba(255,90,90,0.55); }
  .sv-btn-disabled {
    background: #252a3e !important;
    border-color: rgba(106,127,181,0.20) !important;
    box-shadow: none !important;
    opacity: 0.40;
    cursor: not-allowed;
    filter: grayscale(0.6);
    transform: none !important;
  }

  /* ── Item slots ── */
  .sv-slot {
    width: 52px; height: 52px;
    background: #141820;
    border: 1px solid rgba(106,127,181,0.30);
    border-radius: 7px;
    display: flex; align-items: center; justify-content: center;
    position: relative;
    cursor: pointer;
    transition: border-color 0.12s, box-shadow 0.12s;
    box-shadow: inset 0 1px 0 rgba(255,255,255,0.03);
    flex-shrink: 0;
  }
  .sv-slot:hover { border-color: rgba(77,225,255,0.60); box-shadow: 0 0 10px rgba(77,225,255,0.25); }
  .sv-slot-selected { border-color: #4DE1FF !important; animation: crystalPulse 2s infinite; }
  .sv-slot-locked   { background: #101420; border-color: rgba(106,127,181,0.15) !important; cursor: not-allowed; opacity: 0.55; }
  .sv-slot-rare     { border-color: #4F8CFF !important; box-shadow: 0 0 8px rgba(79,140,255,0.38); }
  .sv-slot-epic     { border-color: #9B59B6 !important; box-shadow: 0 0 10px rgba(155,89,182,0.42); background: #1a152c; }
  .sv-slot-legendary{ border-color: #FFD700 !important; background: #1f1a0e; animation: legendaryShimmer 2s infinite; }

  /* ── HUD progress bars ── */
  .sv-bar { height: 8px; border-radius: 4px; background: rgba(0,0,0,0.45); border: 1px solid rgba(106,127,181,0.25); overflow: hidden; }
  .sv-bar-fill { height: 100%; border-radius: 4px; transition: width 0.5s ease; }

  /* ── Notification card ── */
  .sv-notif {
    animation: fadeInUp 0.25s ease-out;
    border-radius: 8px;
    border-left: 3px solid;
    padding: 10px 14px;
    background: rgba(30,34,48,0.96);
    backdrop-filter: blur(10px);
    box-shadow: 0 4px 22px rgba(0,0,0,0.50);
    display: flex; align-items: center; gap: 10px;
    min-width: 260px;
  }

  /* ── Tab nav ── */
  .sv-tab { font-family: 'Press Start 2P', monospace; font-size: 8px; padding: 11px 14px; border-bottom: 2px solid transparent; color: #C7D0E0; transition: all 0.13s; white-space: nowrap; }
  .sv-tab:hover { color: #fff; border-bottom-color: rgba(106,127,181,0.45); }
  .sv-tab-active { color: #4DE1FF !important; border-bottom-color: #4F8CFF !important; background: rgba(79,140,255,0.09); }

  /* ── Scrollbar ── */
  .sv-scroll::-webkit-scrollbar { width: 4px; height: 4px; }
  .sv-scroll::-webkit-scrollbar-track { background: rgba(0,0,0,0.15); border-radius: 2px; }
  .sv-scroll::-webkit-scrollbar-thumb { background: rgba(106,127,181,0.45); border-radius: 2px; }

  /* ── Input field ── */
  .sv-input {
    background: rgba(20,24,38,0.85);
    border: 1px solid rgba(106,127,181,0.40);
    border-radius: 6px;
    padding: 9px 12px;
    color: #fff;
    font-family: 'VT323', monospace;
    font-size: 16px;
    outline: none;
    transition: border-color 0.13s, box-shadow 0.13s;
    width: 100%;
  }
  .sv-input:focus { border-color: #4DE1FF; box-shadow: 0 0 0 2px rgba(77,225,255,0.15); }
  .sv-input::placeholder { color: #6A7FB5; }
`;

// ─── Font helpers ─────────────────────────────────────────────────────────────
const pxFont  = { fontFamily: "'Press Start 2P', monospace" } as const;
const vtFont  = { fontFamily: "'VT323', monospace" } as const;
const monoFont = { fontFamily: "'Share Tech Mono', monospace" } as const;

// ─── Reusable components ─────────────────────────────────────────────────────

function CrystalPanel({ children, className = "", style }: { children: React.ReactNode; className?: string; style?: React.CSSProperties }) {
  return <div className={`sv-panel ${className}`} style={style}>{children}</div>;
}

function PanelHeader({ title, icon, onClose, drag }: { title: string; icon?: React.ReactNode; onClose?: () => void; drag?: boolean }) {
  return (
    <div className={`sv-panel-header ${drag ? "sv-panel-drag" : ""}`}>
      <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
        {icon && <span style={{ color: "#4DE1FF" }}>{icon}</span>}
        <span style={{ ...pxFont, fontSize: 10, color: "#fff", letterSpacing: "0.08em" }}>{title}</span>
      </div>
      {onClose && (
        <button onClick={onClose} style={{ ...pxFont, fontSize: 8, color: "#C7D0E0", background: "none", border: "none", cursor: "pointer", width: 24, height: 24, display: "flex", alignItems: "center", justifyContent: "center", borderRadius: 4, transition: "color 0.12s" }}
          onMouseEnter={e => { (e.currentTarget as HTMLElement).style.color = "#FF5A5A"; (e.currentTarget as HTMLElement).style.background = "rgba(255,90,90,0.1)"; }}
          onMouseLeave={e => { (e.currentTarget as HTMLElement).style.color = "#C7D0E0"; (e.currentTarget as HTMLElement).style.background = "none"; }}>
          ✕
        </button>
      )}
    </div>
  );
}

function Bar({ value, max, color }: { value: number; max: number; color: string }) {
  return (
    <div className="sv-bar" style={{ flex: 1 }}>
      <div className="sv-bar-fill" style={{ width: `${(value / max) * 100}%`, background: color, boxShadow: `0 0 5px ${color}` }} />
    </div>
  );
}

/**
 * LabelBar — progress bar with the value label centered inside the track.
 * Replaces the pattern of placing <icon> <label> <value> above/beside a Bar.
 */
type LabelBarProps = {
  value: number;
  max: number;
  color: string;
  glowColor?: string;
  label: string;           // e.g. "HP", "XP"
  suffix?: string;         // e.g. "HP", "XP", "%" — appended after "cur / max"
  icon?: React.ReactNode;
  height?: number;
  borderColor?: string;
};
function LabelBar({ value, max, color, glowColor, label, suffix = "", icon, height = 20, borderColor }: LabelBarProps) {
  const pct = Math.min(100, Math.max(0, (value / max) * 100));
  const bc  = borderColor ?? `${color}44`;
  const gc  = glowColor   ?? color;
  const text = `${value.toLocaleString()} / ${max.toLocaleString()}${suffix ? " " + suffix : ""}`;
  return (
    <div style={{ position: "relative", height, borderRadius: 5, background: "rgba(0,0,0,0.45)", border: `1px solid ${bc}`, overflow: "hidden", backdropFilter: "blur(2px)" }}>
      {/* Fill */}
      <div style={{
        position: "absolute", inset: 0, width: `${pct}%`,
        background: `linear-gradient(90deg, ${color}cc 0%, ${color} 100%)`,
        boxShadow: `0 0 8px ${gc}88`,
        borderRadius: "5px 0 0 5px",
        transition: "width 0.35s ease",
      }}/>
      {/* Subtle shine stripe */}
      <div style={{ position: "absolute", top: 0, left: 0, right: 0, height: "40%", background: "rgba(255,255,255,0.06)", borderRadius: "5px 5px 0 0", pointerEvents: "none" }}/>
      {/* Centered label */}
      <div style={{ position: "absolute", inset: 0, display: "flex", alignItems: "center", justifyContent: "center", gap: 4 }}>
        {icon && <span style={{ display: "flex", alignItems: "center", flexShrink: 0 }}>{icon}</span>}
        <span style={{
          ...monoFont, fontSize: 9, color: "#fff", letterSpacing: "0.03em",
          textShadow: "0 1px 3px rgba(0,0,0,0.9), 0 0 6px rgba(0,0,0,0.7)",
          whiteSpace: "nowrap",
        }}>
          {text}
        </span>
        <span style={{ ...pxFont, fontSize: 5, color: "rgba(255,255,255,0.65)", textShadow: "0 1px 3px rgba(0,0,0,0.9)" }}>{label}</span>
      </div>
    </div>
  );
}

function Btn({ children, variant = "primary", disabled, size = "md", onClick, style }: {
  children: React.ReactNode;
  variant?: "primary" | "purple" | "success" | "danger" | "disabled";
  disabled?: boolean;
  size?: "sm" | "md" | "lg";
  onClick?: () => void;
  style?: React.CSSProperties;
}) {
  const cls = variant === "purple" ? "sv-btn sv-btn-purple"
    : variant === "success" ? "sv-btn sv-btn-success"
    : variant === "danger" ? "sv-btn sv-btn-danger"
    : variant === "disabled" || disabled ? "sv-btn sv-btn-disabled"
    : "sv-btn";
  const sz: React.CSSProperties = size === "sm" ? { fontSize: 7, padding: "6px 12px" } : size === "lg" ? { fontSize: 11, padding: "12px 24px" } : {};
  return (
    <button className={cls} disabled={disabled || variant === "disabled"} onClick={onClick} style={{ ...sz, ...style }}>
      {children}
    </button>
  );
}

// ─── Tooltip system ──────────────────────────────────────────────────────────
type TooltipItem = {
  name: string;
  description: string;
  rarity: "common" | "rare" | "epic" | "legendary";
  stats?: { label: string; value: string; color?: string }[];
  durability?: { current: number; max: number };
  requirements?: string[];
};

const RARITY_COLOR: Record<string, string> = {
  common: "#C7D0E0", rare: "#4F8CFF", epic: "#9B59B6", legendary: "#FFD700",
};
const RARITY_ORDER: Record<string, number> = {
  common: 0, rare: 1, epic: 2, legendary: 3,
};

function UITooltip({ item, x, y }: { item: TooltipItem; x: number; y: number }) {
  const rc = RARITY_COLOR[item.rarity] ?? "#C7D0E0";
  const durPct = item.durability ? Math.round((item.durability.current / item.durability.max) * 100) : 100;
  const durCol = durPct > 60 ? "#4CD964" : durPct > 30 ? "#FFD54A" : "#FF5A5A";
  return (
    <div className="sv-tooltip sv-panel" style={{ position: "fixed", left: x + 14, top: Math.max(8, y - 10), padding: 12, display: "flex", flexDirection: "column", gap: 6 }}>
      {/* Rarity + name */}
      <div>
        <span style={{ ...pxFont, fontSize: 5, color: rc, background: `${rc}18`, border: `1px solid ${rc}44`, padding: "1px 6px", borderRadius: 3 }}>{item.rarity.toUpperCase()}</span>
        <div style={{ ...pxFont, fontSize: 8, color: rc, marginTop: 5, textShadow: `0 0 10px ${rc}55` }}>{item.name}</div>
      </div>
      {/* Description */}
      <div style={{ ...vtFont, fontSize: 14, color: "#C7D0E0", lineHeight: 1.4 }}>{item.description}</div>
      {/* Stats */}
      {item.stats && item.stats.length > 0 && (
        <div style={{ borderTop: "1px solid rgba(106,127,181,0.22)", paddingTop: 6, display: "flex", flexDirection: "column", gap: 3 }}>
          {item.stats.map((s, i) => (
            <div key={i} style={{ display: "flex", justifyContent: "space-between", gap: 12 }}>
              <span style={{ ...vtFont, fontSize: 13, color: "#6A7FB5" }}>{s.label}</span>
              <span style={{ ...monoFont, fontSize: 10, color: s.color ?? "#fff" }}>{s.value}</span>
            </div>
          ))}
        </div>
      )}
      {/* Durability */}
      {item.durability && (
        <div style={{ borderTop: "1px solid rgba(106,127,181,0.22)", paddingTop: 6 }}>
          <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 4 }}>
            <span style={{ ...pxFont, fontSize: 5, color: "#6A7FB5" }}>DURABILITY</span>
            <span style={{ ...monoFont, fontSize: 9, color: durCol }}>{item.durability.current}/{item.durability.max}</span>
          </div>
          <div className="sv-bar" style={{ height: 5, borderRadius: 3 }}>
            <div className="sv-bar-fill" style={{ width: `${durPct}%`, background: durCol, boxShadow: `0 0 5px ${durCol}88`, borderRadius: 3 }}/>
          </div>
        </div>
      )}
      {/* Requirements */}
      {item.requirements && item.requirements.length > 0 && (
        <div style={{ borderTop: "1px solid rgba(106,127,181,0.22)", paddingTop: 5 }}>
          <div style={{ ...pxFont, fontSize: 5, color: "#FF5A5A", marginBottom: 3 }}>REQUIRES</div>
          {item.requirements.map((r, i) => (
            <div key={i} style={{ ...vtFont, fontSize: 13, color: "#FFD54A", display: "flex", alignItems: "center", gap: 5 }}>
              <span style={{ color: "#FF5A5A" }}>▸</span>{r}
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

// ─── Pixel-art SVG icons ──────────────────────────────────────────────────────
type IconProps = { size?: number; color?: string };

const SvgIcon = {
  Inventory:   ({ size = 16, color = "#4DE1FF" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <rect x="1" y="2" width="14" height="10" rx="1" fill="none" stroke={color} strokeWidth="1.5"/>
      <rect x="3" y="4" width="4" height="3" fill={color} opacity=".55"/>
      <rect x="9" y="4" width="4" height="3" fill={color} opacity=".55"/>
      <rect x="3" y="9" width="6" height="1" fill={color} opacity=".35"/>
      <rect x="5" y="13" width="6" height="2" rx="1" fill={color} opacity=".5"/>
    </svg>
  ),
  Equipment:   ({ size = 16, color = "#4DE1FF" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <path d="M8 1L11 4L9.5 5.5L13 9L11.5 10.5L8 7L6.5 8.5L3 5L4.5 3.5Z" fill={color} opacity=".8"/>
      <rect x="2.5" y="11" width="3" height="4" rx="1" fill={color} opacity=".45"/>
      <rect x="10.5" y="11" width="3" height="4" rx="1" fill={color} opacity=".45"/>
    </svg>
  ),
  Crafting:    ({ size = 16, color = "#4DE1FF" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <rect x="1" y="10" width="4" height="4" rx="1" fill={color} opacity=".6"/>
      <rect x="6" y="10" width="4" height="4" rx="1" fill={color} opacity=".6"/>
      <rect x="11" y="10" width="4" height="4" rx="1" fill={color} opacity=".6"/>
      <rect x="3.5" y="6" width="4" height="4" rx="1" fill={color} opacity=".75"/>
      <rect x="8.5" y="6" width="4" height="4" rx="1" fill={color} opacity=".75"/>
      <rect x="6" y="2" width="4" height="4" rx="1" fill={color}/>
    </svg>
  ),
  Chat:        ({ size = 16, color = "#4DE1FF" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <rect x="1" y="1" width="14" height="10" rx="2" fill="none" stroke={color} strokeWidth="1.5"/>
      <rect x="3" y="4" width="7" height="1.5" fill={color} opacity=".7"/>
      <rect x="3" y="7" width="5" height="1.5" fill={color} opacity=".45"/>
      <polygon points="3,11 3,15 7,11" fill={color} opacity=".6"/>
    </svg>
  ),
  Friends:     ({ size = 16, color = "#4DE1FF" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <circle cx="6" cy="5" r="2.8" fill="none" stroke={color} strokeWidth="1.5"/>
      <path d="M1 14 Q1 10 6 10 Q11 10 11 14" fill="none" stroke={color} strokeWidth="1.5"/>
      <circle cx="12.5" cy="4.5" r="1.8" fill={color} opacity=".5"/>
      <path d="M10 12.5 Q10.5 9.5 12.5 9.5 Q15 9.5 15 12.5" fill="none" stroke={color} strokeWidth="1.5" opacity=".5"/>
    </svg>
  ),
  Guild:       ({ size = 16, color = "#4DE1FF" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <path d="M8 1L15 5V11L8 15L1 11V5Z" fill="none" stroke={color} strokeWidth="1.5"/>
      <path d="M8 4L11 6V10L8 12L5 10V6Z" fill={color} opacity=".25"/>
      <circle cx="8" cy="8" r="1.5" fill={color}/>
    </svg>
  ),
  Mail:        ({ size = 16, color = "#4DE1FF" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <rect x="1" y="3" width="14" height="10" rx="1.5" fill="none" stroke={color} strokeWidth="1.5"/>
      <path d="M1.5 4L8 9.5L14.5 4" fill="none" stroke={color} strokeWidth="1.5"/>
    </svg>
  ),
  Marketplace: ({ size = 16, color = "#FFD700" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <rect x="1" y="7" width="14" height="8" rx="1" fill="none" stroke={color} strokeWidth="1.5"/>
      <path d="M2.5 7L4.5 2H11.5L13.5 7" fill="none" stroke={color} strokeWidth="1.5"/>
      <circle cx="8" cy="11" r="2" fill={color} opacity=".6"/>
    </svg>
  ),
  Settings:    ({ size = 16, color = "#C7D0E0" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <circle cx="8" cy="8" r="2.5" fill="none" stroke={color} strokeWidth="1.5"/>
      <rect x="7" y="1"  width="2" height="2.5" fill={color} opacity=".7"/>
      <rect x="7" y="12.5" width="2" height="2.5" fill={color} opacity=".7"/>
      <rect x="1"  y="7" width="2.5" height="2" fill={color} opacity=".7"/>
      <rect x="12.5" y="7" width="2.5" height="2" fill={color} opacity=".7"/>
      <rect x="3"  y="3"  width="2" height="2" fill={color} opacity=".45"/>
      <rect x="11" y="3"  width="2" height="2" fill={color} opacity=".45"/>
      <rect x="3"  y="11" width="2" height="2" fill={color} opacity=".45"/>
      <rect x="11" y="11" width="2" height="2" fill={color} opacity=".45"/>
    </svg>
  ),
  Quests:      ({ size = 16, color = "#FFD54A" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <path d="M8 1L10 6H15L11 9L13 14L8 11L3 14L5 9L1 6H6Z" fill="none" stroke={color} strokeWidth="1.5"/>
      <path d="M8 4L9.2 7H12.2L10 8.6L10.8 11.6L8 9.8L5.2 11.6L6 8.6L3.8 7H6.8Z" fill={color} opacity=".35"/>
    </svg>
  ),
  Skills:      ({ size = 16, color = "#6C5CE7" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <line x1="8" y1="2"  x2="8"  y2="6"  stroke={color} strokeWidth="2" strokeLinecap="round"/>
      <line x1="8" y1="6"  x2="4"  y2="9"  stroke={color} strokeWidth="2" strokeLinecap="round"/>
      <line x1="8" y1="6"  x2="12" y2="9"  stroke={color} strokeWidth="2" strokeLinecap="round"/>
      <circle cx="8"  cy="2"  r="2"   fill={color}/>
      <circle cx="4"  cy="9"  r="1.8" fill={color} opacity=".65"/>
      <circle cx="12" cy="9"  r="1.8" fill={color} opacity=".65"/>
      <rect x="2"  y="12" width="4" height="3" rx="1" fill={color} opacity=".5"/>
      <rect x="10" y="12" width="4" height="3" rx="1" fill={color} opacity=".5"/>
    </svg>
  ),
  Coins:       ({ size = 16, color = "#FFD700" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <circle cx="8" cy="8" r="6.5" fill="none" stroke={color} strokeWidth="1.5"/>
      <circle cx="8" cy="8" r="4"   fill={color} opacity=".25"/>
      <text x="8" y="11.5" textAnchor="middle" fill={color} style={{ fontSize: 8, fontWeight: 700, fontFamily: "monospace" }}>$</text>
    </svg>
  ),
  Health:      ({ size = 16, color = "#FF5A5A" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <path d="M8 13.5L2 7.5 Q1 3 5 2 Q7 2 8 4.5 Q9 2 11 2 Q15 3 14 7.5Z" fill={color} opacity=".85" stroke={color} strokeWidth=".5"/>
    </svg>
  ),
  Energy:      ({ size = 16, color = "#4DE1FF" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <path d="M9.5 1L4 9H7.5L6 15L13 7H9.5Z" fill={color} opacity=".85" stroke={color} strokeWidth=".5"/>
    </svg>
  ),
  XP:          ({ size = 16, color = "#6C5CE7" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }}>
      <path d="M8 1L10.5 6H15.5L11.5 9.5L13.5 14.5L8 11L2.5 14.5L4.5 9.5L0.5 6H5.5Z" fill={color} opacity=".75" stroke={color} strokeWidth=".5"/>
    </svg>
  ),
  // ── Original crystal gem icon — StrixVerse currency ──
  Gem:         ({ size = 16, color = "#4DE1FF" }: IconProps) => (
    <svg width={size} height={size} viewBox="0 0 16 16" style={{ imageRendering: "pixelated" }} aria-label="Gems">
      {/* Outer facet top */}
      <polygon points="8,1 12,4 8,5.5 4,4" fill={color} opacity=".95"/>
      {/* Left facet */}
      <polygon points="4,4 8,5.5 5,10 2,7" fill={color} opacity=".55"/>
      {/* Right facet */}
      <polygon points="12,4 14,7 11,10 8,5.5" fill={color} opacity=".70"/>
      {/* Center facet */}
      <polygon points="8,5.5 11,10 8,13 5,10" fill={color} opacity=".85"/>
      {/* Bottom tip */}
      <polygon points="5,10 11,10 8,15" fill={color} opacity=".60"/>
      {/* Inner highlight — top-left sparkle */}
      <polygon points="6.5,3 7.8,4.5 6,4.8" fill="#ffffff" opacity=".55"/>
      {/* Small corner sparkles */}
      <rect x="2"  y="2"  width="1" height="1" fill="#ffffff" opacity=".45"/>
      <rect x="13" y="1"  width="1" height="1" fill="#ffffff" opacity=".30"/>
      <rect x="14" y="8"  width="1" height="1" fill="#ffffff" opacity=".25"/>
    </svg>
  ),
};

const ICON_LIST = Object.entries(SvgIcon) as [string, React.FC<IconProps>][];

// ─── Tab definitions ──────────────────────────────────────────────────────────
type TabId = "overview" | "buttons" | "hud" | "windows" | "icons" | "slots" | "notifications" | "screens";
const TABS: { id: TabId; label: string }[] = [
  { id: "overview",      label: "OVERVIEW"      },
  { id: "buttons",       label: "BUTTONS"       },
  { id: "hud",           label: "HUD"           },
  { id: "windows",       label: "WINDOWS"       },
  { id: "icons",         label: "ICONS"         },
  { id: "slots",         label: "ITEM SLOTS"    },
  { id: "notifications", label: "NOTIFICATIONS" },
  { id: "screens",       label: "SCREENS"       },
];

// ─── OVERVIEW TAB ─────────────────────────────────────────────────────────────
const PALETTE = [
  { name: "Background", hex: "#1E2230", role: "Page / Game BG"    },
  { name: "Panel",      hex: "#2C3145", role: "Windows / Panels"  },
  { name: "Primary",    hex: "#4F8CFF", role: "Primary Actions"   },
  { name: "Secondary",  hex: "#6C5CE7", role: "Secondary / Skills"},
  { name: "Highlight",  hex: "#4DE1FF", role: "Crystal Glow"      },
  { name: "Border",     hex: "#6A7FB5", role: "UI Borders"        },
  { name: "Success",    hex: "#4CD964", role: "Health / OK"       },
  { name: "Warning",    hex: "#FFD54A", role: "Quests / Warn"     },
  { name: "Danger",     hex: "#FF5A5A", role: "Errors / Damage"   },
  { name: "Gold",       hex: "#FFD700", role: "Coins / Legendary" },
  { name: "Text",       hex: "#FFFFFF", role: "Primary Text"      },
  { name: "Subtext",    hex: "#C7D0E0", role: "Labels / Captions" },
];

const TYPE_SCALE = [
  { name: "DISPLAY",  font: "'Press Start 2P', monospace", size: "26px", use: "Game title · Splash screen" },
  { name: "HEADER",   font: "'Press Start 2P', monospace", size: "14px", use: "Window titles · Section headings" },
  { name: "LABEL",    font: "'Press Start 2P', monospace", size: "9px",  use: "Button labels · Tab labels" },
  { name: "BODY",     font: "'VT323', monospace",           size: "18px", use: "Tooltips · Chat · Descriptions" },
  { name: "CAPTION",  font: "'VT323', monospace",           size: "14px", use: "Stack counts · Fine print" },
  { name: "DATA",     font: "'Share Tech Mono', monospace", size: "12px", use: "Ping · FPS · Coordinates" },
];

function OverviewTab() {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 24 }}>
      {/* Palette */}
      <CrystalPanel>
        <PanelHeader title="COLOR PALETTE" />
        <div style={{ padding: "20px 20px 24px", display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(120px, 1fr))", gap: 14 }}>
          {PALETTE.map(c => (
            <div key={c.name} style={{ display: "flex", flexDirection: "column", gap: 8 }}>
              <div style={{ height: 56, borderRadius: 7, background: c.hex, border: "1px solid rgba(106,127,181,0.25)", boxShadow: "inset 0 1px 0 rgba(255,255,255,0.08)", display: "flex", alignItems: "flex-end", padding: "0 8px 6px" }}>
                <span style={{ ...monoFont, fontSize: 9, color: "rgba(255,255,255,0.65)" }}>{c.hex}</span>
              </div>
              <div>
                <div style={{ ...pxFont, fontSize: 8, color: "#fff" }}>{c.name}</div>
                <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", marginTop: 2 }}>{c.role}</div>
              </div>
            </div>
          ))}
        </div>
      </CrystalPanel>

      {/* Typography */}
      <CrystalPanel>
        <PanelHeader title="TYPOGRAPHY SCALE" />
        <div style={{ padding: "6px 20px 20px" }}>
          {TYPE_SCALE.map((t, i) => (
            <div key={t.name} style={{ display: "flex", alignItems: "center", gap: 20, padding: "14px 0", borderBottom: i < TYPE_SCALE.length - 1 ? "1px solid rgba(106,127,181,0.14)" : "none" }}>
              <div style={{ width: 72, flexShrink: 0 }}>
                <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF" }}>{t.name}</div>
                <div style={{ ...monoFont, fontSize: 9, color: "#6A7FB5", marginTop: 3 }}>{t.size}</div>
              </div>
              <div style={{ flex: 1, fontFamily: t.font, fontSize: t.size, color: "#fff", textShadow: "1px 1px 0 rgba(0,0,0,0.9)", lineHeight: 1.1 }}>
                StrixVerse Crystal OS
              </div>
              <div style={{ ...vtFont, fontSize: 14, color: "#C7D0E0", flexShrink: 0, textAlign: "right", maxWidth: 220 }}>{t.use}</div>
            </div>
          ))}
        </div>
      </CrystalPanel>

      {/* Spacing */}
      <CrystalPanel>
        <PanelHeader title="SPACING SYSTEM" />
        <div style={{ padding: 20, display: "flex", flexWrap: "wrap", gap: 24, alignItems: "flex-end" }}>
          {[4, 8, 12, 16, 24, 32, 48, 64].map(s => (
            <div key={s} style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 8 }}>
              <div style={{ width: s, height: s, background: "rgba(77,225,255,0.18)", border: "1px solid rgba(77,225,255,0.40)", borderRadius: 3 }} />
              <span style={{ ...monoFont, fontSize: 9, color: "#C7D0E0" }}>{s}px</span>
            </div>
          ))}
        </div>
      </CrystalPanel>
    </div>
  );
}

// ─── BUTTONS TAB ─────────────────────────────────────────────────────────────
function ButtonsTab() {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 20 }}>
      <CrystalPanel>
        <PanelHeader title="BUTTON VARIANTS" />
        <div style={{ padding: 24, display: "flex", flexDirection: "column", gap: 32 }}>

          <section>
            <div style={{ ...pxFont, fontSize: 8, color: "#4DE1FF", marginBottom: 14 }}>VARIANTS</div>
            <div style={{ display: "flex", flexWrap: "wrap", gap: 12, alignItems: "center" }}>
              <Btn>PLAY GAME</Btn>
              <Btn variant="purple">INVENTORY</Btn>
              <Btn variant="success">CONFIRM</Btn>
              <Btn variant="danger">DELETE</Btn>
              <Btn variant="disabled">LOCKED</Btn>
            </div>
          </section>

          <section>
            <div style={{ ...pxFont, fontSize: 8, color: "#4DE1FF", marginBottom: 14 }}>SIZES</div>
            <div style={{ display: "flex", flexWrap: "wrap", gap: 12, alignItems: "flex-end" }}>
              <Btn size="sm">SMALL</Btn>
              <Btn>MEDIUM</Btn>
              <Btn size="lg">LARGE</Btn>
            </div>
          </section>

          <section>
            <div style={{ ...pxFont, fontSize: 8, color: "#4DE1FF", marginBottom: 14 }}>ICON BUTTONS</div>
            <div style={{ display: "flex", flexWrap: "wrap", gap: 10 }}>
              {(["⚔️","🎒","🏠","⚙","💬","👥","📜","🛒"] as const).map((ic, i) => (
                <button key={i} className="sv-btn" style={{ width: 40, height: 40, padding: 0, fontSize: 16 }}>{ic}</button>
              ))}
            </div>
          </section>

          <section>
            <div style={{ ...pxFont, fontSize: 8, color: "#4DE1FF", marginBottom: 14 }}>STATES</div>
            <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(150px, 1fr))", gap: 16 }}>
              {[
                { label: "IDLE", note: "Default resting state", extra: {} },
                { label: "HOVER", note: "On mouse enter", extra: { background: "linear-gradient(180deg,#4a5e8e,#3c4b70)", borderColor: "rgba(77,225,255,.80)", boxShadow: "0 0 16px rgba(77,225,255,.50)" } },
                { label: "PRESSED", note: "Click / keypress", extra: { transform: "translateY(1px)", boxShadow: "0 0 4px rgba(77,225,255,.18)" } },
                { label: "DISABLED", note: "Not available", variant: "disabled" as const },
                { label: "SUCCESS", note: "Positive action", variant: "success" as const },
                { label: "DANGER", note: "Destructive action", variant: "danger" as const },
              ].map(s => (
                <div key={s.label} style={{ display: "flex", flexDirection: "column", alignItems: "flex-start", gap: 8 }}>
                  <Btn variant={s.variant} style={s.extra as React.CSSProperties}>{s.label}</Btn>
                  <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>{s.note}</span>
                </div>
              ))}
            </div>
          </section>

        </div>
      </CrystalPanel>
    </div>
  );
}

// ─── CURRENCY SYSTEM ─────────────────────────────────────────────────────────

/** Format a gem/currency value: use compact notation above 10 000 */
function formatCurrency(n: number): string {
  if (n >= 1_000_000) return (n / 1_000_000).toFixed(1).replace(/\.0$/, "") + "M";
  if (n >= 10_000)    return (n / 1_000).toFixed(1).replace(/\.0$/, "") + "K";
  return n.toLocaleString();
}

type FloatEntry = { id: number; delta: number; x: number; y: number };

/**
 * Generic reusable currency display widget.
 * Pass any icon, color and value — works for Gems, Coins, Tokens, etc.
 * `animClass` is toggled from outside to drive earn/spend CSS animations.
 */
type CurrencyDisplayProps = {
  icon: React.ReactNode;
  value: number;
  color: string;
  glowColor: string;
  label: string;
  animClass: string;
  style?: React.CSSProperties;
};
function CurrencyDisplay({ icon, value, color, glowColor, label, animClass, style }: CurrencyDisplayProps) {
  return (
    <div
      title={label}
      className={animClass || undefined}
      style={{
        display: "flex", alignItems: "center", gap: 6,
        background: "rgba(14,18,30,0.82)",
        border: `1px solid ${glowColor}44`,
        borderRadius: 6, padding: "5px 10px",
        backdropFilter: "blur(4px)",
        boxShadow: `0 0 8px ${glowColor}22`,
        cursor: "default",
        ...style,
      }}>
      {icon}
      <span style={{ ...monoFont, fontSize: 11, color, letterSpacing: "0.02em", minWidth: 28, textAlign: "right" }}>
        {formatCurrency(value)}
      </span>
    </div>
  );
}

/** Floating +X / -X notification that drifts upward and fades. */
function GemFloatNotif({ entry }: { entry: FloatEntry }) {
  const earn = entry.delta > 0;
  return (
    <div
      key={entry.id}
      className="sv-gem-float"
      style={{
        left: entry.x,
        top: entry.y,
        color: earn ? "#4DE1FF" : "#FF5A5A",
      }}>
      {earn ? "+" : ""}{formatCurrency(entry.delta)} 💎
    </div>
  );
}

// ─── HUD TAB ─────────────────────────────────────────────────────────────────
const CHAT_MSGS = [
  { user: "CrystalKnight", msg: "Anyone want to trade rare shards?", col: "#4DE1FF", ch: "WORLD" },
  { user: "PixelMage",     msg: "I found a Legendary Gem!!",         col: "#FFD700", ch: "WORLD" },
  { user: "StoneWarden",   msg: "New world open: Vortex-7",          col: "#4CD964", ch: "GLOBAL"},
  { user: "System",        msg: "Crystal event starts in 4 min",      col: "#FFD54A", ch: "SYSTEM"},
  { user: "GhostRider",    msg: "GG everyone, nice run",             col: "#C7D0E0", ch: "WORLD" },
  { user: "ArcaneWitch",   msg: "LFG dungeon run? DM me",            col: "#9B59B6", ch: "WORLD" },
];
const HOTBAR = ["⚔️","🪄","🏹","🛡️","💊","💎","🔮","🧪","🗝️"];
const BUFFS = [
  { icon:"✨", label:"ATK+", col:"#FFD700", secs:42 },
  { icon:"🛡️", label:"DEF+", col:"#4F8CFF", secs:28 },
  { icon:"💨", label:"SPD+", col:"#4DE1FF", secs:15 },
  { icon:"☠️", label:"POISON",col:"#4CD964", secs:8, bad:true },
];
const QUICK_BTNS = [
  { icon:"🎒", label:"INV"   },
  { icon:"⚔️", label:"EQUIP" },
  { icon:"🔨", label:"CRAFT" },
  { icon:"👥", label:"FRND"  },
  { icon:"🏰", label:"GUILD" },
  { icon:"🛒", label:"MRKT"  },
  { icon:"✉️", label:"MAIL"  },
  { icon:"⚙️", label:"SET"   },
];

type HUDTabProps = {
  coins: number;
  gems: number;
  gemAnim: string;
  floats: FloatEntry[];
  onCoinsClick: () => void;
  onGemsClick: () => void;
  triggerGemChange: (delta: number) => void;
};

function HUDTab({ coins, gems, gemAnim, floats, onCoinsClick, onGemsClick, triggerGemChange }: HUDTabProps) {
  const [chatCh, setChatCh] = useState<"WORLD"|"GLOBAL"|"PARTY"|"GUILD">("WORLD");
  const [quickOpen, setQuickOpen] = useState(false);
  const [selectedSlot, setSelectedSlot] = useState(0);

  const SELECTED_ITEM = { name: "Crystal Blade", dur: 78 };

  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 16 }}>
      <CrystalPanel>
        <PanelHeader title="GAMEPLAY HUD — SIDE-VIEW 2D SANDBOX (NO MINIMAP)" />
        <div style={{ padding: 16 }}>

          {/* ── VIEWPORT FRAME ── */}
          <div className="sv-pixel-grid" style={{ height: 560, borderRadius: 10, background: "linear-gradient(180deg,#0a1018 0%,#0f1c2a 40%,#071018 100%)", position: "relative", overflow: "hidden" }}>

            {/* Subtle scanline overlay */}
            <div style={{ position: "absolute", inset: 0, backgroundImage: "repeating-linear-gradient(0deg, transparent, transparent 3px, rgba(0,0,0,0.04) 3px, rgba(0,0,0,0.04) 4px)", pointerEvents: "none", zIndex: 1 }}/>

            {/* Viewport watermark */}
            <div style={{ position: "absolute", inset: 0, display: "flex", alignItems: "center", justifyContent: "center", pointerEvents: "none", zIndex: 0 }}>
              <span style={{ ...pxFont, fontSize: 9, color: "rgba(255,255,255,0.04)", letterSpacing: "0.2em" }}>GAME WORLD VIEWPORT</span>
            </div>

            {/* ───────────────────── TOP LEFT: Player Stats ───────────────────── */}
            <div style={{ position: "absolute", top: 12, left: 12, zIndex: 10, display: "flex", flexDirection: "column", gap: 6 }}>
              {/* Name + level row */}
              <div style={{ display: "flex", alignItems: "center", gap: 8, background: "rgba(14,18,30,0.78)", border: "1px solid rgba(106,127,181,0.28)", borderRadius: 6, padding: "5px 10px", backdropFilter: "blur(4px)" }}>
                <span style={{ fontSize: 18 }}>🧙</span>
                <span style={{ ...pxFont, fontSize: 7, color: "#fff" }}>CrystalMage</span>
                <span style={{ ...monoFont, fontSize: 9, color: "#4DE1FF", background: "rgba(77,225,255,0.12)", padding: "1px 5px", borderRadius: 3 }}>Lv.42</span>
              </div>
              {/* HP — value embedded inside bar */}
              <LabelBar
                value={375} max={500}
                color="#4CD964" glowColor="#4CD964"
                label="HP"
                icon={<SvgIcon.Health size={10} color="#FF5A5A"/>}
                height={22}
                borderColor="rgba(76,217,100,0.35)"
              />
              {/* XP — value embedded inside bar */}
              <LabelBar
                value={4200} max={10000}
                color="#6C5CE7" glowColor="#8b6ff5"
                label="XP"
                icon={<SvgIcon.XP size={10} color="#6C5CE7"/>}
                height={22}
                borderColor="rgba(108,92,231,0.35)"
              />
              {/* Active buffs/debuffs */}
              <div style={{ display: "flex", gap: 5 }}>
                {BUFFS.map((b, i) => (
                  <div key={i} title={b.label} style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 2, background: b.bad ? "rgba(255,90,90,0.14)" : "rgba(14,18,30,0.72)", border: `1px solid ${b.col}44`, borderRadius: 5, padding: "3px 5px", minWidth: 30 }}>
                    <span style={{ fontSize: 10 }}>{b.icon}</span>
                    <span style={{ ...monoFont, fontSize: 7, color: b.col }}>{b.secs}s</span>
                  </div>
                ))}
              </div>
            </div>

            {/* ───────────────────── TOP RIGHT: World Info + Gems + Status ───────────────────── */}
            <div style={{ position: "absolute", top: 12, right: 12, zIndex: 10, display: "flex", flexDirection: "column", alignItems: "flex-end", gap: 5 }}>
              {/* ── COINS — click opens Wallet ── */}
              <div
                title="Coins — click to open Wallet"
                style={{ cursor: "pointer" }}
                onClick={onCoinsClick}
                onMouseEnter={e => { (e.currentTarget as HTMLElement).style.filter = "brightness(1.18)"; }}
                onMouseLeave={e => { (e.currentTarget as HTMLElement).style.filter = ""; }}>
                <CurrencyDisplay
                  icon={<SvgIcon.Coins size={14} color="#FFD700"/>}
                  value={coins}
                  color="#FFD700"
                  glowColor="#FFD700"
                  label="Coins — opens Wallet"
                  animClass=""
                />
              </div>

              {/* ── GEMS — click opens Premium Shop ── */}
              <div style={{ position: "relative" }}>
                <div
                  title="Gems — click to open Premium Shop"
                  style={{ cursor: "pointer" }}
                  onClick={onGemsClick}
                  onMouseEnter={e => { (e.currentTarget as HTMLElement).style.filter = "brightness(1.18)"; }}
                  onMouseLeave={e => { (e.currentTarget as HTMLElement).style.filter = ""; }}>
                  <CurrencyDisplay
                    icon={<SvgIcon.Gem size={14} color="#4DE1FF"/>}
                    value={gems}
                    color="#4DE1FF"
                    glowColor="#4DE1FF"
                    label="Gems — opens Premium Shop"
                    animClass={gemAnim}
                  />
                </div>
                {/* Floating +/- notifications */}
                {floats.map(e => (
                  <div key={e.id} className="sv-gem-float" style={{ right: 0, top: -4, color: e.delta > 0 ? "#4DE1FF" : "#FF5A5A" }}>
                    {e.delta > 0 ? "+" : ""}{formatCurrency(e.delta)} 💎
                  </div>
                ))}
              </div>

              {/* Stats row: Ping, FPS, Coords */}
              <div style={{ background: "rgba(14,18,30,0.72)", border: "1px solid rgba(106,127,181,0.25)", borderRadius: 6, padding: "6px 12px", backdropFilter: "blur(4px)", display: "flex", flexDirection: "column", gap: 3, alignItems: "flex-end" }}>
                <div style={{ display: "flex", gap: 16 }}>
                  <span style={{ ...monoFont, fontSize: 9, color: "#4CD964" }}>PING</span>
                  <span style={{ ...monoFont, fontSize: 9, color: "#4CD964" }}>24 ms</span>
                </div>
                <div style={{ display: "flex", gap: 16 }}>
                  <span style={{ ...monoFont, fontSize: 9, color: "#4DE1FF" }}>FPS</span>
                  <span style={{ ...monoFont, fontSize: 9, color: "#4DE1FF" }}>60</span>
                </div>
                <div style={{ height: 1, background: "rgba(106,127,181,0.25)", alignSelf: "stretch", margin: "1px 0" }}/>
                <div style={{ display: "flex", gap: 6, alignItems: "center" }}>
                  <span style={{ ...monoFont, fontSize: 8, color: "rgba(199,208,224,0.5)" }}>X:1842 Y:64</span>
                </div>
              </div>
              {/* Notification bell */}
              <div style={{ background: "rgba(14,18,30,0.72)", border: "1px solid rgba(106,127,181,0.25)", borderRadius: 6, padding: "5px 10px", backdropFilter: "blur(4px)", display: "flex", alignItems: "center", gap: 6, cursor: "pointer" }}>
                <span style={{ fontSize: 12 }}>🔔</span>
                <span style={{ ...pxFont, fontSize: 6, color: "#FFD54A", background: "rgba(255,213,74,0.18)", padding: "1px 5px", borderRadius: 3 }}>3</span>
              </div>
            </div>

            {/* ───────────────────── RIGHT EDGE: Quick Access Buttons (collapsible) ───────────────────── */}
            <div style={{ position: "absolute", top: "50%", right: 12, transform: "translateY(-50%)", zIndex: 10, display: "flex", flexDirection: "column", gap: 4 }}>
              <button onClick={() => setQuickOpen(o => !o)} style={{ background: quickOpen ? "rgba(79,140,255,0.25)" : "rgba(14,18,30,0.75)", border: "1px solid rgba(79,140,255,0.40)", borderRadius: 5, padding: "4px 6px", cursor: "pointer", ...pxFont, fontSize: 7, color: "#4DE1FF", backdropFilter: "blur(4px)" }}>
                {quickOpen ? "▶" : "◀"}
              </button>
              {quickOpen && QUICK_BTNS.map((b, i) => (
                <button key={i} title={b.label} style={{ width: 36, height: 36, background: "rgba(14,18,30,0.82)", border: "1px solid rgba(106,127,181,0.32)", borderRadius: 5, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 1, cursor: "pointer", backdropFilter: "blur(4px)", transition: "border-color .12s" }}
                  onMouseEnter={e => { (e.currentTarget as HTMLElement).style.borderColor = "rgba(77,225,255,0.55)"; }}
                  onMouseLeave={e => { (e.currentTarget as HTMLElement).style.borderColor = "rgba(106,127,181,0.32)"; }}>
                  <span style={{ fontSize: 14 }}>{b.icon}</span>
                  <span style={{ ...pxFont, fontSize: 4, color: "#C7D0E0" }}>{b.label}</span>
                </button>
              ))}
            </div>

            {/* ───────────────────── BOTTOM LEFT: Chat ───────────────────── */}
            <div style={{ position: "absolute", bottom: 58, left: 12, width: 240, zIndex: 10 }}>
              <div style={{ background: "rgba(14,18,30,0.82)", border: "1px solid rgba(106,127,181,0.30)", borderRadius: 8, backdropFilter: "blur(6px)", overflow: "hidden" }}>
                {/* Channel tabs */}
                <div style={{ display: "flex", borderBottom: "1px solid rgba(106,127,181,0.18)" }}>
                  {(["WORLD","GLOBAL","PARTY","GUILD"] as const).map(ch => (
                    <button key={ch} onClick={() => setChatCh(ch)}
                      style={{ flex: 1, ...pxFont, fontSize: 5, padding: "5px 2px", cursor: "pointer", background: chatCh === ch ? "rgba(79,140,255,0.16)" : "transparent", color: chatCh === ch ? "#4DE1FF" : "#6A7FB5", border: "none", borderBottom: chatCh === ch ? "2px solid #4DE1FF" : "2px solid transparent", transition: "all .12s" }}>
                      {ch}
                    </button>
                  ))}
                </div>
                {/* Messages */}
                <div className="sv-scroll" style={{ padding: "6px 10px", display: "flex", flexDirection: "column", gap: 3, maxHeight: 82, overflowY: "auto" }}>
                  {CHAT_MSGS.filter(m => m.ch === chatCh || chatCh === "GLOBAL").slice(0, 6).map((m, i) => (
                    <div key={i} style={{ ...vtFont, fontSize: 13, lineHeight: 1.2 }}>
                      <span style={{ color: m.col }}>[{m.user}]</span>{" "}
                      <span style={{ color: "#fff" }}>{m.msg}</span>
                    </div>
                  ))}
                </div>
                {/* Input row */}
                <div style={{ padding: "4px 8px", borderTop: "1px solid rgba(106,127,181,0.18)", display: "flex", gap: 4, alignItems: "center" }}>
                  <div style={{ flex: 1, background: "rgba(0,0,0,0.35)", border: "1px solid rgba(106,127,181,0.25)", borderRadius: 4, padding: "3px 7px", ...vtFont, fontSize: 12, color: "#6A7FB5" }}>
                    Press T to chat...
                  </div>
                  {/* Quick emotes */}
                  {["😄","👍","👋","🎉"].map((e, i) => (
                    <button key={i} style={{ background: "none", border: "none", cursor: "pointer", fontSize: 13, padding: 2, borderRadius: 3, transition: "background .1s" }}
                      onMouseEnter={el => { (el.currentTarget as HTMLElement).style.background = "rgba(255,255,255,0.1)"; }}
                      onMouseLeave={el => { (el.currentTarget as HTMLElement).style.background = "none"; }}>
                      {e}
                    </button>
                  ))}
                </div>
              </div>
            </div>

            {/* ───────────────────── BOTTOM RIGHT: Objectives + Interaction ───────────────────── */}
            <div style={{ position: "absolute", bottom: 58, right: quickOpen ? 58 : 12, width: 200, zIndex: 10, display: "flex", flexDirection: "column", gap: 5, transition: "right .2s" }}>
              {/* Interaction prompt */}
              <div style={{ background: "rgba(14,18,30,0.88)", border: "1px solid rgba(77,225,255,0.35)", borderRadius: 7, padding: "7px 12px", display: "flex", alignItems: "center", gap: 8, boxShadow: "0 0 12px rgba(77,225,255,0.12)", backdropFilter: "blur(4px)" }}>
                <div style={{ width: 22, height: 22, borderRadius: 4, background: "rgba(79,140,255,0.20)", border: "1px solid rgba(79,140,255,0.50)", display: "flex", alignItems: "center", justifyContent: "center" }}>
                  <span style={{ ...pxFont, fontSize: 7, color: "#4DE1FF" }}>E</span>
                </div>
                <span style={{ ...vtFont, fontSize: 14, color: "#fff" }}>Open Crystal Chest</span>
              </div>
              {/* Active objectives */}
              <div style={{ background: "rgba(14,18,30,0.80)", border: "1px solid rgba(106,127,181,0.28)", borderRadius: 7, backdropFilter: "blur(4px)", overflow: "hidden" }}>
                <div style={{ padding: "5px 10px", borderBottom: "1px solid rgba(106,127,181,0.18)", display: "flex", alignItems: "center", gap: 6 }}>
                  <span style={{ ...pxFont, fontSize: 5, color: "#FFD54A" }}>📋</span>
                  <span style={{ ...pxFont, fontSize: 5, color: "#FFD54A" }}>OBJECTIVES</span>
                </div>
                <div style={{ padding: "7px 10px", display: "flex", flexDirection: "column", gap: 7 }}>
                  {[
                    { name: "Gather Crystal Shards", prog: 60, cur: 6,  max: 10 },
                    { name: "Defeat Stone Golem",    prog: 0,  cur: 0,  max: 1  },
                  ].map((q, i) => (
                    <div key={i}>
                      <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 3 }}>
                        <span style={{ ...vtFont, fontSize: 13, color: "#fff" }}>{q.name}</span>
                        <span style={{ ...monoFont, fontSize: 8, color: "#C7D0E0" }}>{q.cur}/{q.max}</span>
                      </div>
                      <div className="sv-bar">
                        <div className="sv-bar-fill" style={{ width: `${q.prog}%`, background: "#FFD54A", boxShadow: "0 0 4px #FFD54A44" }}/>
                      </div>
                    </div>
                  ))}
                </div>
              </div>
              {/* Temp notification */}
              <div style={{ background: "rgba(76,217,100,0.12)", border: "1px solid rgba(76,217,100,0.35)", borderRadius: 7, padding: "6px 10px", display: "flex", alignItems: "center", gap: 8, animation: "fadeInUp .3s ease-out", backdropFilter: "blur(4px)" }}>
                <span style={{ fontSize: 14 }}>⭐</span>
                <div>
                  <div style={{ ...pxFont, fontSize: 5, color: "#4CD964" }}>QUEST COMPLETE</div>
                  <div style={{ ...vtFont, fontSize: 12, color: "#C7D0E0" }}>The Crystal Hunt · +500 XP</div>
                </div>
              </div>
            </div>

            {/* ───────────────────── BOTTOM CENTER: Hotbar ───────────────────── */}
            <div style={{ position: "absolute", bottom: 8, left: "50%", transform: "translateX(-50%)", zIndex: 10, display: "flex", flexDirection: "column", alignItems: "center", gap: 4 }}>
              {/* Selected item label + durability */}
              <div style={{ display: "flex", alignItems: "center", gap: 8, background: "rgba(14,18,30,0.78)", border: "1px solid rgba(106,127,181,0.25)", borderRadius: 5, padding: "3px 12px", backdropFilter: "blur(4px)" }}>
                <span style={{ ...pxFont, fontSize: 6, color: "#4DE1FF" }}>{SELECTED_ITEM.name}</span>
                <div style={{ display: "flex", alignItems: "center", gap: 5 }}>
                  <span style={{ ...monoFont, fontSize: 7, color: "#C7D0E0" }}>DUR</span>
                  <div style={{ width: 50, height: 4, borderRadius: 2, background: "rgba(0,0,0,0.4)", border: "1px solid rgba(106,127,181,0.25)", overflow: "hidden" }}>
                    <div style={{ height: "100%", width: `${SELECTED_ITEM.dur}%`, background: "#4CD964", borderRadius: 2 }}/>
                  </div>
                  <span style={{ ...monoFont, fontSize: 7, color: "#4CD964" }}>{SELECTED_ITEM.dur}%</span>
                </div>
              </div>
              {/* Slots */}
              <div style={{ display: "flex", gap: 4, background: "rgba(14,18,30,0.72)", border: "1px solid rgba(106,127,181,0.28)", borderRadius: 8, padding: "4px 6px", backdropFilter: "blur(6px)" }}>
                {HOTBAR.map((item, i) => (
                  <div key={i}
                    className={`sv-slot ${i === selectedSlot ? "sv-slot-selected" : ""}`}
                    style={{ width: 44, height: 44, cursor: "pointer" }}
                    onClick={() => setSelectedSlot(i)}>
                    <span style={{ fontSize: 18 }}>{item}</span>
                    <span style={{ ...pxFont, position: "absolute", top: 2, left: 3, fontSize: 5, color: i === selectedSlot ? "#4DE1FF" : "#6A7FB5" }}>{i + 1}</span>
                  </div>
                ))}
              </div>
            </div>

          </div>{/* end viewport frame */}
        </div>
      </CrystalPanel>

      {/* ── GEMS INTERACTIVE DEMO ── */}
      <CrystalPanel>
        <PanelHeader title="GEMS CURRENCY — LIVE DEMO"/>
        <div style={{ padding: 20 }}>
          <div style={{ display: "flex", flexWrap: "wrap", gap: 20, alignItems: "flex-start" }}>

            {/* Live display */}
            <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 10 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 2 }}>CURRENT BALANCE</div>
              <div style={{ position: "relative", display: "inline-block" }}>
                <CurrencyDisplay
                  icon={<SvgIcon.Gem size={20} color="#4DE1FF"/>}
                  value={gems}
                  color="#4DE1FF"
                  glowColor="#4DE1FF"
                  label="Gems"
                  animClass={gemAnim}
                  style={{ padding: "8px 16px", borderRadius: 8, fontSize: 14 }}
                />
                {floats.map(e => (
                  <div key={e.id} className="sv-gem-float" style={{ right: 8, top: 0, color: e.delta > 0 ? "#4DE1FF" : "#FF5A5A" }}>
                    {e.delta > 0 ? "+" : ""}{formatCurrency(e.delta)} 💎
                  </div>
                ))}
              </div>
              <div style={{ ...vtFont, fontSize: 13, color: "#6A7FB5" }}>Value always from server</div>
            </div>

            {/* Trigger buttons — simulate server packets */}
            <div style={{ flex: 1, minWidth: 280 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 10 }}>SIMULATE SERVER PACKET</div>
              <div style={{ display: "flex", flexWrap: "wrap", gap: 8 }}>
                {[
                  { label:"Farm Block",       delta:+1,    icon:"⛏️" },
                  { label:"Quest Reward",      delta:+250,  icon:"⭐" },
                  { label:"Sell Item",         delta:+840,  icon:"🛒" },
                  { label:"Level Up Bonus",    delta:+5000, icon:"✨" },
                  { label:"Trade Receive",     delta:+120,  icon:"🔄" },
                  { label:"Use Gem Spell",     delta:-25,   icon:"🔮" },
                  { label:"Buy Cosmetic",      delta:-800,  icon:"🎨" },
                  { label:"Marketplace Buy",   delta:-1200, icon:"💸" },
                ].map(({ label, delta, icon }) => (
                  <button key={label} onClick={() => triggerGemChange(delta)}
                    className={`sv-btn ${delta < 0 ? "sv-btn-danger" : "sv-btn-success"}`}
                    style={{ fontSize: 7, padding: "6px 10px", display: "flex", alignItems: "center", gap: 5 }}>
                    <span>{icon}</span>
                    <span>{label}</span>
                    <span style={{ opacity: 0.7 }}>({delta > 0 ? "+" : ""}{formatCurrency(delta)})</span>
                  </button>
                ))}
              </div>
            </div>

            {/* Format examples */}
            <div style={{ minWidth: 180 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 10 }}>FORMAT EXAMPLES</div>
              <div style={{ display: "flex", flexDirection: "column", gap: 6 }}>
                {[
                  { raw: 0,          fmt: "0"     },
                  { raw: 125,        fmt: "125"   },
                  { raw: 5420,       fmt: "5,420" },
                  { raw: 10000,      fmt: "10K"   },
                  { raw: 58400,      fmt: "58.4K" },
                  { raw: 1200000,    fmt: "1.2M"  },
                ].map(({ raw, fmt }) => (
                  <div key={raw} style={{ display: "flex", alignItems: "center", gap: 8 }}>
                    <CurrencyDisplay
                      icon={<SvgIcon.Gem size={11} color="#4DE1FF"/>}
                      value={raw}
                      color="#4DE1FF"
                      glowColor="#4DE1FF"
                      label="Gems"
                      animClass=""
                      style={{ padding: "3px 8px", fontSize: 10 }}
                    />
                    <span style={{ ...monoFont, fontSize: 9, color: "#6A7FB5" }}>→ {fmt}</span>
                  </div>
                ))}
              </div>
            </div>

          </div>

          {/* Currency system extensibility note */}
          <div style={{ marginTop: 18, padding: "10px 14px", background: "rgba(79,140,255,0.07)", border: "1px solid rgba(79,140,255,0.22)", borderRadius: 7 }}>
            <div style={{ ...pxFont, fontSize: 6, color: "#4DE1FF", marginBottom: 6 }}>EXTENSIBLE CURRENCY SYSTEM</div>
            <div style={{ display: "flex", gap: 10, flexWrap: "wrap" }}>
              {[
                { icon:<SvgIcon.Gem size={12} color="#4DE1FF"/>,   label:"Gems",         val:5420,   color:"#4DE1FF" },
                { icon:<SvgIcon.Coins size={12} color="#FFD700"/>, label:"Gold",         val:4280,   color:"#FFD700" },
                { icon:<span style={{fontSize:12}}>🎟️</span>,       label:"Event Tokens", val:88,     color:"#FF5A5A" },
                { icon:<span style={{fontSize:12}}>🏰</span>,       label:"Guild Points", val:12400,  color:"#6C5CE7" },
                { icon:<span style={{fontSize:12}}>⚔️</span>,       label:"Arena Points", val:2300,   color:"#FFD54A" },
              ].map(c => (
                <CurrencyDisplay
                  key={c.label}
                  icon={c.icon}
                  value={c.val}
                  color={c.color}
                  glowColor={c.color}
                  label={c.label}
                  animClass=""
                />
              ))}
            </div>
            <div style={{ ...vtFont, fontSize: 13, color: "#6A7FB5", marginTop: 8 }}>
              All currencies share the same <code style={{ ...monoFont, fontSize: 11, color: "#4DE1FF", background: "rgba(77,225,255,0.10)", padding: "1px 5px", borderRadius: 3 }}>CurrencyDisplay</code> component. Add new ones by passing a different icon, color and server-synced value.
            </div>
          </div>
        </div>
      </CrystalPanel>

      {/* Spec notes */}
      <CrystalPanel>
        <PanelHeader title="HUD DESIGN NOTES"/>
        <div style={{ padding: 20, display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(240px,1fr))", gap: 12 }}>
          {[
            { icon:"❌", title:"NO MINIMAP", note:"StrixVerse is a side-view sandbox. Players explore naturally via landmarks, portals, signs, and coordinate sharing." },
            { icon:"◀▶", title:"QUICK ACCESS", note:"8 icon buttons collapse behind a toggle arrow on the right edge. Unobtrusive during gameplay, instant access on demand." },
            { icon:"💬", title:"CHAT CHANNELS", note:"World · Global · Party · Guild — tabbed within the chat window. Channel indicator always visible." },
            { icon:"⌨️", title:"INTERACTION PROMPT", note:"Context-sensitive 'Press E' prompt appears above the objective tracker when an interactable is nearby." },
            { icon:"📊", title:"COMPACT STATS", note:"HP and XP bars stack vertically top-left. Energy bar removed — not used in current gameplay. Buffs & debuffs appear as small timed icons below." },
            { icon:"🎯", title:"HOTBAR SLOTS", note:"9 active slots. Selected slot highlighted in cyan. Item name and durability shown above the bar." },
          ].map(n => (
            <div key={n.title} style={{ background: "rgba(14,18,30,0.55)", border: "1px solid rgba(106,127,181,0.20)", borderRadius: 7, padding: "10px 14px" }}>
              <div style={{ display: "flex", alignItems: "center", gap: 6, marginBottom: 5 }}>
                <span style={{ fontSize: 14 }}>{n.icon}</span>
                <span style={{ ...pxFont, fontSize: 6, color: "#4DE1FF" }}>{n.title}</span>
              </div>
              <span style={{ ...vtFont, fontSize: 14, color: "#C7D0E0", lineHeight: 1.45 }}>{n.note}</span>
            </div>
          ))}
        </div>
      </CrystalPanel>
    </div>
  );
}

// ─── WINDOWS TAB ─────────────────────────────────────────────────────────────
type WinTab = "inventory" | "equipment" | "appearance" | "crafting" | "npc" | "storage" | "trading" | "marketplace" | "friends" | "guild" | "mail" | "quest" | "wallet" | "shop" | "settings";

type InvItem = { emoji: string; name: string; rarity: "common"|"rare"|"epic"|"legendary"; qty: number; desc?: string; stats?: { label: string; value: string; color?: string }[]; durability?: { current: number; max: number }; requirements?: string[] };
const INV_ITEMS: (InvItem | null)[] = [
  { emoji:"💎", name:"Crystal Shard",  rarity:"rare",      qty:24,  desc:"Raw crystal fragment harvested from deep mines.", stats:[{label:"Material Grade",value:"★★★",color:"#4F8CFF"}] },
  { emoji:"⚔️", name:"Crystal Blade",  rarity:"epic",      qty:1,   desc:"Forged from raw crystal ore. Hums with arcane energy.", stats:[{label:"Damage",value:"+145",color:"#4DE1FF"},{label:"Crit Chance",value:"+8%",color:"#FFD54A"},{label:"Attack Speed",value:"1.4/s",color:"#C7D0E0"}], durability:{current:78,max:100}, requirements:["Level 40","Warrior Class"] },
  { emoji:"🧪", name:"Mana Potion",    rarity:"common",    qty:5,   desc:"Restores 80 MP on use.", stats:[{label:"MP Restore",value:"80",color:"#6C5CE7"},{label:"Cooldown",value:"30s",color:"#C7D0E0"}] },
  { emoji:"🔮", name:"Arcane Orb",     rarity:"legendary", qty:1,   desc:"Ancient artifact pulsing with unstable magic.", stats:[{label:"Magic Power",value:"+220",color:"#FFD700"},{label:"Int Bonus",value:"+35",color:"#4DE1FF"},{label:"Spell Haste",value:"+12%",color:"#FFD54A"}], durability:{current:100,max:100}, requirements:["Level 50","Mage Class"] },
  { emoji:"🛡️", name:"Stone Shield",   rarity:"common",    qty:1,   desc:"Basic defensive shield carved from granite.", stats:[{label:"Defense",value:"+42",color:"#4CD964"},{label:"Block Rate",value:"18%",color:"#C7D0E0"}], durability:{current:55,max:80} },
  { emoji:"🪙", name:"Gold Coins",     rarity:"common",    qty:999, desc:"Standard currency of the realm. Spend at shops and the marketplace." },
  { emoji:"🗝️", name:"Dungeon Key",    rarity:"rare",      qty:3,   desc:"Unlocks a sealed dungeon entrance. Single use.", stats:[{label:"Dungeon Tier",value:"III",color:"#4F8CFF"}] },
  { emoji:"📜", name:"Quest Scroll",   rarity:"common",    qty:2,   desc:"Read to begin an archived quest." },
  { emoji:"🌿", name:"Herb Bundle",    rarity:"common",    qty:12,  desc:"Gathered herbs for alchemy and cooking.", stats:[{label:"Alchemy Grade",value:"B",color:"#4CD964"}] },
  { emoji:"💊", name:"Health Vial",    rarity:"common",    qty:8,   desc:"Restores 50 HP on use.", stats:[{label:"HP Restore",value:"50",color:"#4CD964"},{label:"Cooldown",value:"15s",color:"#C7D0E0"}] },
  { emoji:"🪄", name:"Mage Staff",     rarity:"epic",      qty:1,   desc:"Channeling rod carved from Crystalwood.", stats:[{label:"Magic Power",value:"+160",color:"#9B59B6"},{label:"Cast Speed",value:"+10%",color:"#4DE1FF"},{label:"Mana Regen",value:"+5/s",color:"#6C5CE7"}], durability:{current:92,max:100}, requirements:["Level 38","Mage Class"] },
  { emoji:"🎭", name:"Crystal Mask",   rarity:"rare",      qty:1,   desc:"A ceremonial mask that warps perception.", stats:[{label:"Stealth",value:"+25",color:"#4F8CFF"},{label:"Luck",value:"+8",color:"#FFD54A"}], durability:{current:100,max:100} },
  null, null, null, null, null, null,
  null, null, null, null, null, null,
  null, null, null, null, null, null,
];

const RARITY_CLS: Record<string, string> = {
  common:    "",
  rare:      "sv-slot-rare",
  epic:      "sv-slot-epic",
  legendary: "sv-slot-legendary",
};

// ── Shop data ────────────────────────────────────────────────────────────────
type ShopSection = "coin" | "gem";
type ConfirmItem = { name: string; emoji: string; price: number; currency: "coin" | "gem"; desc: string } | null;

const COIN_CATS = ["Featured","World Essentials","Blocks","Seeds","Farming","Tools","Crafting","Furniture","Utility","Miscellaneous"] as const;
const GEM_CATS  = ["Featured Premium","New Arrivals","Cosmetics","Clothing","Hats","Wings","Pets","Decorations","Bundles","Seasonal","Limited Time"] as const;

type CoinCat = typeof COIN_CATS[number];
type GemCat  = typeof GEM_CATS[number];

const COIN_ITEMS: Record<CoinCat, { emoji:string; name:string; desc:string; price:number; badge?:string }[]> = {
  "Featured":         [
    { emoji:"🔒", name:"World Lock",        desc:"Lock your world to protect it.",     price:50000, badge:"ESSENTIAL" },
    { emoji:"🏠", name:"Small Lock",         desc:"Lock a small area securely.",        price:5000  },
    { emoji:"⚒️", name:"Crystal Crafting Station",desc:"Advanced crafting bench.",     price:8500, badge:"POPULAR"  },
    { emoji:"🌾", name:"Watering Can",       desc:"Waters 3×3 crop area.",             price:300   },
    { emoji:"🎣", name:"Fishing Rod",        desc:"Cast a line in any water tile.",     price:250   },
  ],
  "World Essentials": [
    { emoji:"🔒", name:"World Lock",         desc:"Lock your world completely.",        price:50000, badge:"ESSENTIAL" },
    { emoji:"🏠", name:"Small Lock",         desc:"Lock a 10×10 area.",                price:5000  },
    { emoji:"🚪", name:"Door",               desc:"Lockable door for your build.",      price:200   },
    { emoji:"🗺️", name:"World Map",          desc:"See your world from above.",         price:1000  },
    { emoji:"⭐", name:"Star Statue",         desc:"Decoration for your world.",         price:3000  },
  ],
  "Blocks":           [
    { emoji:"🟦", name:"Crystal Block",      desc:"Gleaming blue crystal.",             price:50    },
    { emoji:"🪨", name:"Stone Block",        desc:"Solid foundation block.",            price:10    },
    { emoji:"🪵", name:"Wood Plank",         desc:"Classic building material.",         price:15    },
    { emoji:"🔲", name:"Glass Pane",         desc:"Transparent decorative block.",      price:30    },
    { emoji:"🟫", name:"Dirt Block",         desc:"Basic natural block.",               price:5     },
    { emoji:"🏜️", name:"Sand Block",         desc:"Beach and desert terrain.",          price:8     },
    { emoji:"🧱", name:"Brick Block",        desc:"Sturdy decorative brick.",           price:25    },
    { emoji:"❄️", name:"Ice Block",          desc:"Slippery crystal-clear ice.",        price:40    },
  ],
  "Seeds":            [
    { emoji:"🥕", name:"Carrot Seed",        desc:"Grows into a carrot in 2 days.",     price:50    },
    { emoji:"🌾", name:"Wheat Seed",         desc:"Grows into wheat in 1 day.",         price:40    },
    { emoji:"🌻", name:"Sunflower Seed",     desc:"Decorative blooming flower.",        price:80    },
    { emoji:"💎", name:"Crystal Seed",       desc:"Rare gem-bearing plant.",            price:500, badge:"RARE"   },
    { emoji:"🍄", name:"Mushroom Spore",     desc:"Grows in dark, damp areas.",         price:60    },
    { emoji:"🌹", name:"Rose Seed",          desc:"Beautiful red rose bush.",           price:70    },
  ],
  "Farming":          [
    { emoji:"🚿", name:"Watering Can",       desc:"Waters a 3×3 crop area.",           price:300   },
    { emoji:"🌿", name:"Fertilizer",         desc:"Doubles crop growth speed.",         price:150   },
    { emoji:"⛏️", name:"Hoe",               desc:"Tills soil for planting.",           price:200   },
    { emoji:"🎠", name:"Scarecrow",          desc:"Keeps crows away from crops.",       price:800   },
    { emoji:"🪣", name:"Irrigation Bucket",  desc:"Waters a single tile.",              price:80    },
    { emoji:"📦", name:"Compost Bin",        desc:"Turn waste into fertilizer.",        price:600   },
  ],
  "Tools":            [
    { emoji:"⛏️", name:"Iron Pickaxe",       desc:"Mines stone and ore efficiently.",   price:400   },
    { emoji:"⚔️", name:"Iron Sword",         desc:"Basic combat weapon.",               price:350   },
    { emoji:"🎣", name:"Fishing Rod",        desc:"Fish in any body of water.",         price:250   },
    { emoji:"🔦", name:"Lantern",            desc:"Provides area lighting.",            price:180   },
    { emoji:"🗡️", name:"Dagger",            desc:"Fast attack speed weapon.",           price:300   },
    { emoji:"🏹", name:"Bow",                desc:"Ranged weapon, needs arrows.",        price:450   },
    { emoji:"🪓", name:"Axe",               desc:"Chops wood tiles fast.",             price:280   },
    { emoji:"🔱", name:"Crystal Pickaxe",    desc:"Mines hard crystal deposits.",       price:1200, badge:"BEST"   },
  ],
  "Crafting":         [
    { emoji:"🔨", name:"Crafting Table",     desc:"Basic item crafting.",               price:1000  },
    { emoji:"🔥", name:"Furnace",            desc:"Smelt ores into ingots.",            price:1500  },
    { emoji:"⚒️", name:"Anvil",             desc:"Repair and upgrade gear.",           price:2000  },
    { emoji:"📚", name:"Enchanting Table",   desc:"Enchant gear with magic.",           price:5000, badge:"ADVANCED"},
    { emoji:"🧪", name:"Alchemy Lab",        desc:"Brew powerful potions.",             price:3500  },
    { emoji:"🪚", name:"Sawmill",            desc:"Process wood into planks.",          price:1800  },
  ],
  "Furniture":        [
    { emoji:"🪑", name:"Wooden Chair",       desc:"Sit and relax.",                     price:120   },
    { emoji:"🛏️", name:"Bed",               desc:"Save spawn point.",                  price:500   },
    { emoji:"📚", name:"Bookshelf",          desc:"Decorative storage.",                price:300   },
    { emoji:"📦", name:"Chest",              desc:"Stores 20 item stacks.",             price:400   },
    { emoji:"🕯️", name:"Candelabra",        desc:"Elegant light source.",              price:220   },
    { emoji:"🖼️", name:"Painting",          desc:"Art for your walls.",                price:150   },
    { emoji:"🪞", name:"Mirror",             desc:"Decorate your home.",                price:180   },
    { emoji:"🌿", name:"Flower Pot",         desc:"Indoor plant decoration.",           price:80    },
  ],
  "Utility":          [
    { emoji:"🪢", name:"Rope",               desc:"Climb up vertical surfaces.",        price:100   },
    { emoji:"🔦", name:"Torch",              desc:"Small light source.",                price:20    },
    { emoji:"🪜", name:"Ladder",             desc:"Vertical movement aid.",             price:30    },
    { emoji:"🪣", name:"Bucket",             desc:"Carry liquids.",                     price:80    },
    { emoji:"🔑", name:"Key",                desc:"Opens locked containers.",           price:150   },
    { emoji:"💊", name:"Health Potion",      desc:"Restores 50 HP instantly.",          price:120   },
    { emoji:"🔮", name:"Mana Orb",           desc:"Restores 40 MP instantly.",          price:130   },
    { emoji:"💡", name:"Signal Light",       desc:"Colored alert beacon.",              price:60    },
  ],
  "Miscellaneous":    [
    { emoji:"🎁", name:"Random Box",         desc:"Contains a random common item.",     price:500   },
    { emoji:"🏷️", name:"Name Tag",           desc:"Rename tamed pets.",                 price:200   },
    { emoji:"📜", name:"Lore Scroll",        desc:"World lore collectible.",            price:350   },
    { emoji:"🎵", name:"Music Box",          desc:"Plays background music.",            price:450   },
  ],
};

const GEM_ITEMS: Record<GemCat, { emoji:string; name:string; desc:string; price:number; badge?:string }[]> = {
  "Featured Premium": [
    { emoji:"🦋", name:"Crystal Founder's Pack", desc:"Wings + Crown + Aura + Title + Blocks.", price:1200, badge:"LIMITED" },
    { emoji:"🐉", name:"Mini Drake Pet",          desc:"Loyal flying companion.",               price:400,  badge:"NEW"     },
    { emoji:"🌸", name:"Spring Blossom Bundle",   desc:"Hat + Wings + Aura + Emote.",           price:800,  badge:"SEASONAL"},
  ],
  "New Arrivals":     [
    { emoji:"🐉", name:"Mini Drake Pet",          desc:"Loyal flying dragon companion.",        price:400,  badge:"NEW"    },
    { emoji:"🌊", name:"Ocean Aura",              desc:"Swirling water effect.",                price:200,  badge:"NEW"    },
    { emoji:"👒", name:"Adventurer Hat",          desc:"Classic explorer style.",               price:120,  badge:"NEW"    },
    { emoji:"💃", name:"Crystal Dance",           desc:"Glittering dance emote.",               price:90,   badge:"NEW"    },
  ],
  "Cosmetics":        [
    { emoji:"✨", name:"Arcane Aura",             desc:"Swirling magical energy.",              price:250   },
    { emoji:"🌊", name:"Ocean Aura",              desc:"Flowing water aura.",                   price:200   },
    { emoji:"🔥", name:"Flame Aura",              desc:"Intense fire surrounding.",             price:280   },
    { emoji:"❄️", name:"Frost Aura",              desc:"Chilling ice crystal trail.",           price:220   },
    { emoji:"⚡", name:"Storm Aura",              desc:"Electric energy crackle.",              price:260   },
  ],
  "Clothing":         [
    { emoji:"🧥", name:"Crystal Robe",            desc:"Elegant magical outfit.",               price:350   },
    { emoji:"👗", name:"Wardrobe Expansion +5",   desc:"5 extra wardrobe slots.",               price:300   },
    { emoji:"👕", name:"Pixel T-Shirt Pack",      desc:"5 stylish graphic tees.",               price:150   },
    { emoji:"🥻", name:"Elven Dress",             desc:"Flowing forest spirit gown.",           price:280   },
    { emoji:"🧣", name:"Crystal Scarf",           desc:"Shimmering neckwear.",                  price:100   },
  ],
  "Hats":             [
    { emoji:"🎩", name:"Shadow Hood",             desc:"Mysterious dark hood.",                 price:150   },
    { emoji:"👑", name:"Crown Hat",               desc:"Royal golden crown.",                   price:380   },
    { emoji:"🪖", name:"Crystal Helm",            desc:"Decorative crystal helmet.",            price:200   },
    { emoji:"👒", name:"Adventurer Hat",          desc:"Wide-brim explorer hat.",               price:120   },
    { emoji:"🎓", name:"Scholar Cap",             desc:"Academic mortarboard.",                 price:100   },
    { emoji:"🧢", name:"Crystal Cap",             desc:"Casual gem-studded cap.",               price:90    },
  ],
  "Wings":            [
    { emoji:"🦋", name:"Crystal Wings",           desc:"Shimmering blue butterfly wings.",      price:300, badge:"POPULAR" },
    { emoji:"🦅", name:"Eagle Wings",             desc:"Powerful eagle-feather wings.",         price:350   },
    { emoji:"😇", name:"Angel Wings",             desc:"Pure white divine wings.",              price:400   },
    { emoji:"🦇", name:"Shadow Wings",            desc:"Dark bat-like wings.",                  price:320   },
    { emoji:"🐉", name:"Dragon Wings",            desc:"Ancient dragon wing membranes.",        price:450, badge:"RARE"    },
  ],
  "Pets":             [
    { emoji:"🐉", name:"Mini Drake",              desc:"Loyal flying dragon.",                  price:400, badge:"NEW"    },
    { emoji:"🐱", name:"Crystal Cat",             desc:"Glowing gem-eyed cat.",                 price:280   },
    { emoji:"🦊", name:"Spirit Fox",              desc:"Magical ethereal fox.",                 price:350   },
    { emoji:"🐦", name:"Pixel Parrot",            desc:"Colorful talking companion.",           price:200   },
    { emoji:"🐻", name:"Tiny Bear",              desc:"Adorable miniature bear.",              price:260   },
    { emoji:"🦋", name:"Fairy Wisp",             desc:"Glowing magical sprite.",               price:320   },
  ],
  "Decorations":      [
    { emoji:"🪑", name:"Crystal Throne",          desc:"Majestic seat of power.",               price:180   },
    { emoji:"🏮", name:"Crystal Lantern",         desc:"Glowing decorative light.",             price:120   },
    { emoji:"🌺", name:"Enchanted Garden Set",    desc:"5 magical plant decorations.",          price:250   },
    { emoji:"🗿", name:"Crystal Statue",          desc:"Gleaming monument piece.",              price:300   },
    { emoji:"🎆", name:"Firework Emitter",        desc:"Launches visual fireworks.",            price:200   },
  ],
  "Bundles":          [
    { emoji:"🦋", name:"Crystal Founder's Pack",  desc:"Wings + Crown + Aura + Title.",         price:1200, badge:"BEST VALUE" },
    { emoji:"🌸", name:"Spring Bundle",           desc:"Hat + Wings + Aura + Emote.",           price:800,  badge:"SEASONAL"   },
    { emoji:"🐉", name:"Dragon Bundle",           desc:"Dragon Wings + Drake Pet + Aura.",      price:950   },
    { emoji:"⭐", name:"Starter Pack",             desc:"Basic hat + outfit + pet.",             price:450   },
  ],
  "Seasonal":         [
    { emoji:"🌸", name:"Spring Blossom Bundle",   desc:"Seasonal spring collection.",           price:800, badge:"SEASONAL"   },
    { emoji:"🎃", name:"Harvest Hat",             desc:"Spooky seasonal headwear.",             price:140, badge:"SEASONAL"   },
    { emoji:"❄️", name:"Winter Wings",            desc:"Snowflake-patterned wings.",            price:320, badge:"SEASONAL"   },
    { emoji:"🎄", name:"Festive Cape",            desc:"Holiday season collectible cape.",      price:200, badge:"SEASONAL"   },
  ],
  "Limited Time":     [
    { emoji:"🦋", name:"Crystal Founder's Pack",  desc:"Last chance — never sold again.",       price:1200, badge:"LIMITED" },
    { emoji:"🌊", name:"Void Aura",               desc:"Extremely rare void energy.",           price:600,  badge:"LIMITED" },
    { emoji:"👑", name:"Legendary Crown",         desc:"One-of-a-kind royal crown.",            price:900,  badge:"LIMITED" },
  ],
};

function WindowsTab({ subTab, setSubTab }: { subTab: WinTab; setSubTab: (t: WinTab) => void }) {
  // Shop state
  const [shopSection, setShopSection] = useState<ShopSection>("coin");
  const [coinCat, setCoinCat] = useState<CoinCat>("Featured");
  const [gemCat, setGemCat] = useState<GemCat>("Featured Premium");
  const [shopSearch, setShopSearch] = useState("");
  const [confirmItem, setConfirmItem] = useState<ConfirmItem>(null);
  // Tooltip state
  const [tooltip, setTooltip] = useState<TooltipItem | null>(null);
  const [tooltipPos, setTooltipPos] = useState({ x: 0, y: 0 });
  // Inventory search + sort
  const [invSearch, setInvSearch] = useState("");
  const [invSort, setInvSort] = useState<"default"|"name"|"rarity"|"qty">("default");
  // Storage search + sort
  const [storageSearch, setStorageSearch] = useState("");
  const [storageSort, setStorageSort] = useState<"default"|"name">("default");
  // Settings tab
  const [settingsTab, setSettingsTab] = useState("GRAPHICS");

  const SUB: { id: WinTab; label: string; accent?: string }[] = [
    { id: "inventory",   label: "INVENTORY"  },
    { id: "equipment",   label: "EQUIPMENT"  },
    { id: "appearance",  label: "WARDROBE"   },
    { id: "crafting",    label: "CRAFTING"   },
    { id: "storage",     label: "STORAGE"    },
    { id: "trading",     label: "TRADING"    },
    { id: "marketplace", label: "MARKET"     },
    { id: "friends",     label: "FRIENDS"    },
    { id: "guild",       label: "GUILD"      },
    { id: "mail",        label: "MAIL"       },
    { id: "quest",       label: "QUEST LOG"  },
    { id: "wallet",      label: "💰 WALLET",  accent: "#FFD700" },
    { id: "shop",        label: "🏪 SHOP",    accent: "#4DE1FF" },
    { id: "npc",         label: "NPC"        },
    { id: "settings",    label: "⚙ SETTINGS", accent: "#C7D0E0" },
  ];
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
      {/* Sub-tab bar */}
      <div style={{ display: "flex", gap: 4, flexWrap: "wrap" }}>
        {SUB.map(t => {
          const active = subTab === t.id;
          const ac = t.accent ?? "#4F8CFF";
          return (
            <button key={t.id} onClick={() => setSubTab(t.id)}
              className={`sv-tab ${active ? "sv-tab-active" : ""}`}
              style={{
                background: active ? `${ac}18` : "rgba(44,49,69,0.55)",
                borderRadius: "8px 8px 0 0",
                borderTop: `1px solid ${active ? ac + "55" : "rgba(106,127,181,0.28)"}`,
                borderLeft: `1px solid ${active ? ac + "55" : "rgba(106,127,181,0.28)"}`,
                borderRight: `1px solid ${active ? ac + "55" : "rgba(106,127,181,0.28)"}`,
                borderBottom: "none",
                color: active && t.accent ? t.accent : undefined,
              }}>
              {t.label}
            </button>
          );
        })}
      </div>

      {/* ── INVENTORY ── */}
      {subTab === "inventory" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <div className="sv-panel-header sv-panel-drag">
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <SvgIcon.Inventory size={14} color="#4DE1FF"/>
              <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>INVENTORY</span>
            </div>
            <div style={{ display: "flex", alignItems: "center", gap: 16 }}>
              <span style={{ ...vtFont, fontSize: 15, color: "#FFD700" }}>🪙 4,280</span>
              <span style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>12/30 slots</span>
              <button className="sv-btn sv-btn-disabled" style={{ width: 24, height: 24, padding: 0, fontSize: 9 }}>✕</button>
            </div>
          </div>
          {/* Filter + Search + Sort row */}
          <div style={{ padding: "10px 16px 0", display: "flex", gap: 4, alignItems: "center", flexWrap: "wrap" }}>
            {["ALL","EQUIP","USE","MATS","MISC"].map((f, i) => (
              <button key={f} style={{ ...vtFont, fontSize: 14, padding: "4px 12px", borderRadius: 5, cursor: "pointer", background: i === 0 ? "rgba(79,140,255,0.20)" : "transparent", color: i === 0 ? "#4DE1FF" : "#C7D0E0", border: i === 0 ? "1px solid rgba(79,140,255,0.40)" : "1px solid transparent", transition: "all .12s" }}
                onMouseEnter={e => { if (i !== 0) (e.currentTarget as HTMLElement).style.color = "#fff"; }}
                onMouseLeave={e => { if (i !== 0) (e.currentTarget as HTMLElement).style.color = "#C7D0E0"; }}>
                {f}
              </button>
            ))}
            <div style={{ flex: 1, minWidth: 100 }}/>
            <input className="sv-input" placeholder="Search items..." value={invSearch} onChange={e => setInvSearch(e.target.value)} style={{ width: 130, padding: "3px 8px", fontSize: 13 }}/>
            {(["default","name","rarity","qty"] as const).map(s => (
              <button key={s} onClick={() => setInvSort(s)} style={{ ...vtFont, fontSize: 13, padding: "3px 8px", borderRadius: 4, cursor: "pointer", background: invSort === s ? "rgba(77,225,255,0.14)" : "transparent", color: invSort === s ? "#4DE1FF" : "#C7D0E0", border: invSort === s ? "1px solid rgba(77,225,255,0.40)" : "1px solid transparent", transition: "all .12s", textTransform: "uppercase" }}>
                {s}
              </button>
            ))}
          </div>
          {(() => {
            const named = INV_ITEMS.filter(Boolean) as NonNullable<typeof INV_ITEMS[0]>[];
            let filtered = invSearch ? named.filter(it => it.name.toLowerCase().includes(invSearch.toLowerCase())) : named;
            if (invSort === "name") filtered = [...filtered].sort((a, b) => a.name.localeCompare(b.name));
            else if (invSort === "rarity") filtered = [...filtered].sort((a, b) => RARITY_ORDER[b.rarity] - RARITY_ORDER[a.rarity]);
            else if (invSort === "qty") filtered = [...filtered].sort((a, b) => b.qty - a.qty);
            const displayedItems: (typeof INV_ITEMS[0])[] = [...filtered];
            while (displayedItems.length < 30) displayedItems.push(null);
            return (
          <div style={{ padding: 16, display: "flex", gap: 16 }}>
            {/* Grid */}
            <div style={{ display: "grid", gridTemplateColumns: "repeat(6, 52px)", gap: 5, flexShrink: 0 }}>
              {displayedItems.map((item, i) => (
                <div key={i} className={`sv-slot ${item ? RARITY_CLS[item.rarity] : ""}`}
                  onMouseEnter={item?.desc ? (e) => { setTooltip({ name: item.name, description: item.desc!, rarity: item.rarity, stats: item.stats, durability: item.durability, requirements: item.requirements }); setTooltipPos({ x: e.clientX, y: e.clientY }); } : undefined}
                  onMouseMove={item?.desc ? (e) => setTooltipPos({ x: e.clientX, y: e.clientY }) : undefined}
                  onMouseLeave={item?.desc ? () => setTooltip(null) : undefined}>
                  {item && <span style={{ fontSize: 20 }}>{item.emoji}</span>}
                  {item && item.qty > 1 && <span style={{ ...pxFont, position: "absolute", bottom: 1, right: 2, fontSize: 5, color: "#FFD700" }}>{item.qty > 99 ? "99+" : item.qty}</span>}
                </div>
              ))}
            </div>
            {/* Detail sidebar */}
            <div style={{ flex: 1, minWidth: 150 }}>
              <div style={{ background: "rgba(18,22,38,0.85)", border: "1px solid rgba(155,89,182,0.40)", borderRadius: 8, padding: 14, boxShadow: "0 0 16px rgba(155,89,182,0.18)" }}>
                <div style={{ display: "flex", alignItems: "center", gap: 10, marginBottom: 12 }}>
                  <span style={{ fontSize: 36 }}>⚔️</span>
                  <div>
                    <div style={{ ...pxFont, fontSize: 8, color: "#9B59B6" }}>Crystal Blade</div>
                    <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", marginTop: 2 }}>EPIC WEAPON</div>
                  </div>
                </div>
                <div style={{ borderTop: "1px solid rgba(106,127,181,0.20)", paddingTop: 10, display: "flex", flexDirection: "column", gap: 5 }}>
                  {[
                    { label: "Damage", val: "+145", col: "#4DE1FF" },
                    { label: "Speed",  val: "+12%", col: "#4DE1FF" },
                    { label: "Crit",   val: "+8%",  col: "#FFD54A" },
                    { label: "Level",  val: "Req 40",col: "#C7D0E0"},
                  ].map(s => (
                    <div key={s.label} style={{ display: "flex", justifyContent: "space-between" }}>
                      <span style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>{s.label}</span>
                      <span style={{ ...vtFont, fontSize: 14, color: s.col }}>{s.val}</span>
                    </div>
                  ))}
                </div>
                <div style={{ marginTop: 10 }}>
                  <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 3 }}>
                    <span style={{ ...vtFont, fontSize: 12, color: "#C7D0E0" }}>Durability</span>
                    <span style={{ ...vtFont, fontSize: 12, color: "#4CD964" }}>78/100</span>
                  </div>
                  <div className="sv-bar"><div className="sv-bar-fill" style={{ width: "78%", background: "#4CD964" }}/></div>
                </div>
                <div style={{ marginTop: 12, display: "flex", gap: 8 }}>
                  <Btn size="sm" style={{ flex: 1 }}>EQUIP</Btn>
                  <Btn size="sm" variant="danger" style={{ flex: 1 }}>DROP</Btn>
                </div>
                <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", marginTop: 10, padding: "8px", background: "rgba(0,0,0,0.2)", borderRadius: 5, lineHeight: 1.4 }}>
                  A blade forged from pure arcane crystal. Resonates with magical energy.
                </div>
              </div>
            </div>
          </div>
            );
          })()}
        </CrystalPanel>
      )}

      {/* ── EQUIPMENT ── */}
      {subTab === "equipment" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <PanelHeader title="EQUIPMENT" icon={<SvgIcon.Equipment size={14}/>} onClose={() => {}} drag/>
          <div style={{ padding: 20, display: "flex", gap: 20 }}>

            {/* Character silhouette + slots */}
            <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#C7D0E0", marginBottom: 2 }}>EQUIPPED GEAR</div>
              {/* 3-column slot grid: left col = Head/Shirt/Pants/Shoes/Gloves, center = char, right = Face/MainHand/OffHand/BackItem/Pet */}
              <div style={{ display: "grid", gridTemplateColumns: "68px 80px 68px", gridTemplateRows: "repeat(5,68px)", gap: 6, alignItems: "center" }}>

                {/* Left column */}
                {[
                  { label:"HEAD",   emoji:"🪖", row:1 },
                  { label:"SHIRT",  emoji:"🧥", row:2 },
                  { label:"PANTS",  emoji:"👖", row:3 },
                  { label:"SHOES",  emoji:"👟", row:4 },
                  { label:"GLOVES", emoji:"🧤", row:5 },
                ].map(s => (
                  <div key={s.label} className="sv-slot sv-slot-rare" style={{ width: 64, height: 64, flexDirection: "column", gap: 2, gridColumn: 1, gridRow: s.row }}>
                    <span style={{ fontSize: 22 }}>{s.emoji}</span>
                    <span style={{ ...pxFont, fontSize: 5, color: "#C7D0E0" }}>{s.label}</span>
                  </div>
                ))}

                {/* Center — character preview */}
                <div style={{ gridColumn: 2, gridRow: "1 / 6", display: "flex", alignItems: "center", justifyContent: "center", borderRadius: 10, background: "rgba(14,18,30,0.75)", border: "1px solid rgba(106,127,181,0.25)", fontSize: 52, boxShadow: "inset 0 0 20px rgba(77,225,255,0.06)" }}>
                  🧙
                </div>

                {/* Right column */}
                {[
                  { label:"FACE",      emoji:"😶", row:1 },
                  { label:"MAIN HAND", emoji:"⚔️", row:2 },
                  { label:"OFF HAND",  emoji:"🛡️", row:3 },
                  { label:"BACK",      emoji:"🎒", row:4 },
                  { label:"PET",       emoji:"🐉", row:5 },
                ].map(s => (
                  <div key={s.label} className="sv-slot sv-slot-epic" style={{ width: 64, height: 64, flexDirection: "column", gap: 2, gridColumn: 3, gridRow: s.row }}>
                    <span style={{ fontSize: 22 }}>{s.emoji}</span>
                    <span style={{ ...pxFont, fontSize: 5, color: "#C7D0E0" }}>{s.label}</span>
                  </div>
                ))}
              </div>
            </div>

            {/* Stats panel */}
            <div style={{ flex: 1, minWidth: 200 }}>
              <div style={{ ...pxFont, fontSize: 8, color: "#4DE1FF", marginBottom: 14 }}>CHARACTER STATS</div>
              <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
                {[
                  { stat:"STR", val:84, color:"#FF5A5A", desc:"Attack power"  },
                  { stat:"DEF", val:62, color:"#4F8CFF", desc:"Damage resist"  },
                  { stat:"INT", val:91, color:"#6C5CE7", desc:"Magic potency"  },
                  { stat:"AGI", val:73, color:"#4DE1FF", desc:"Move & attack speed" },
                  { stat:"LCK", val:45, color:"#FFD700", desc:"Drop rate bonus"},
                  { stat:"VIT", val:78, color:"#4CD964", desc:"Max health pool" },
                ].map(s => (
                  <div key={s.stat}>
                    <div style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: 3 }}>
                      <span style={{ ...pxFont, fontSize: 8, color: "#C7D0E0", width: 28 }}>{s.stat}</span>
                      <div className="sv-bar" style={{ flex: 1, height: 10 }}>
                        <div className="sv-bar-fill" style={{ width: `${s.val}%`, background: s.color, boxShadow: `0 0 4px ${s.color}` }}/>
                      </div>
                      <span style={{ ...monoFont, fontSize: 10, color: "#fff", width: 22, textAlign: "right" }}>{s.val}</span>
                    </div>
                    <div style={{ ...vtFont, fontSize: 11, color: "#6A7FB5", paddingLeft: 36 }}>{s.desc}</div>
                  </div>
                ))}
              </div>
              <div style={{ marginTop: 18, padding: 14, background: "rgba(14,18,30,0.75)", borderRadius: 8, border: "1px solid rgba(106,127,181,0.22)" }}>
                <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 10 }}>SET BONUSES</div>
                {[
                  { label:"Crystal Armor (2/4)", val:"+15% Crystal DMG",  col:"#FFD54A" },
                  { label:"Total Power",          val:"3,240",             col:"#4DE1FF" },
                  { label:"Combat Rating",         val:"Epic",             col:"#9B59B6" },
                  { label:"Gear Score",            val:"412 / 500",        col:"#C7D0E0" },
                ].map(b => (
                  <div key={b.label} style={{ display: "flex", justifyContent: "space-between", marginBottom: 6 }}>
                    <span style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>{b.label}</span>
                    <span style={{ ...vtFont, fontSize: 14, color: b.col }}>{b.val}</span>
                  </div>
                ))}
              </div>
              <div style={{ marginTop: 10, display: "flex", gap: 8 }}>
                <Btn size="sm" style={{ flex: 1 }}>COMPARE</Btn>
                <Btn size="sm" variant="purple" style={{ flex: 1 }}>WARDROBE</Btn>
              </div>
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── APPEARANCE / WARDROBE ── */}
      {subTab === "appearance" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <div className="sv-panel-header sv-panel-drag">
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <span style={{ color: "#6C5CE7", fontSize: 14 }}>✨</span>
              <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>WARDROBE</span>
              <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", marginLeft: 6 }}>Cosmetic Appearance</span>
            </div>
            <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
              <span style={{ ...vtFont, fontSize: 13, color: "#4DE1FF" }}>Appearance ≠ Stats</span>
              <button className="sv-btn sv-btn-disabled" style={{ width: 24, height: 24, padding: 0, fontSize: 9 }}>✕</button>
            </div>
          </div>

          {/* Info banner */}
          <div style={{ margin: "10px 16px 0", padding: "8px 14px", background: "rgba(108,92,231,0.10)", border: "1px solid rgba(108,92,231,0.30)", borderRadius: 7, display: "flex", alignItems: "center", gap: 8 }}>
            <span style={{ fontSize: 14 }}>💡</span>
            <span style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>
              Cosmetics are earned through gameplay, quests, crafting, and the marketplace. They never affect your combat statistics.
            </span>
          </div>

          <div style={{ padding: 16, display: "flex", gap: 16 }}>
            {/* Character preview */}
            <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 10, flexShrink: 0 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#C7D0E0" }}>PREVIEW</div>
              <div style={{ width: 120, height: 200, borderRadius: 12, background: "rgba(14,18,30,0.80)", border: "1px solid rgba(108,92,231,0.35)", display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 4, boxShadow: "0 0 20px rgba(108,92,231,0.15)" }}>
                <span style={{ fontSize: 56 }}>🧙</span>
                <div style={{ display: "flex", gap: 4 }}>
                  <span style={{ fontSize: 18 }}>🦋</span>
                  <span style={{ fontSize: 18 }}>✨</span>
                </div>
              </div>
              <div style={{ ...pxFont, fontSize: 7, color: "#6C5CE7" }}>CrystalMage</div>
              <Btn size="sm" variant="purple">PREVIEW ON</Btn>
            </div>

            {/* Cosmetic slot grid — scrollable */}
            <div style={{ flex: 1 }}>
              <div className="sv-scroll" style={{ display: "flex", flexDirection: "column", gap: 6, maxHeight: 380, overflowY: "auto" }}>
                {[
                  { cat:"HAIR STYLE",       icon:"💇", equipped:"Crystal Waves",       unlocked:["Crystal Waves","Shadow Spikes","Wild Flame","Classic Short","Elegant Bun"], color:"#4DE1FF" },
                  { cat:"HAT",              icon:"🎩", equipped:"Crystal Crown",        unlocked:["Crystal Crown","Mage Hood","Adventurer Cap","Witch Hat"], color:"#6C5CE7" },
                  { cat:"FACE ACCESSORY",   icon:"🕶️", equipped:"None",                unlocked:["Arcane Monocle","Crystal Specs","War Paint"], color:"#C7D0E0" },
                  { cat:"SHIRT SKIN",       icon:"👕", equipped:"Azure Robe",           unlocked:["Azure Robe","Shadow Wrap","Forest Tunic"], color:"#4F8CFF" },
                  { cat:"PANTS SKIN",       icon:"👖", equipped:"Crystal Trousers",     unlocked:["Crystal Trousers","Rogue Leggings"], color:"#4F8CFF" },
                  { cat:"SHOES SKIN",       icon:"👟", equipped:"Arcane Boots",          unlocked:["Arcane Boots","Light Sandals","Shadow Steps"], color:"#4F8CFF" },
                  { cat:"GLOVES SKIN",      icon:"🧤", equipped:"None",                unlocked:["Mage Wraps","Iron Gauntlets"], color:"#C7D0E0" },
                  { cat:"BACK COSMETICS",   icon:"🎒", equipped:"Crystal Wings Idle",  unlocked:["Crystal Wings Idle","Cloak of Dusk","Feather Mantle"], color:"#FFD700" },
                  { cat:"WINGS",            icon:"🦋", equipped:"Crystal Butterfly",   unlocked:["Crystal Butterfly","Shadow Wings","Angel Wings"], color:"#4DE1FF" },
                  { cat:"AURA",             icon:"✨", equipped:"Arcane Shimmer",        unlocked:["Arcane Shimmer","Shadow Aura","Golden Light"], color:"#FFD700" },
                  { cat:"TITLE",            icon:"📛", equipped:"Crystal Collector",    unlocked:["Crystal Collector","World Builder","Legendary Hero"], color:"#FFD54A" },
                  { cat:"PET",              icon:"🐉", equipped:"Mini Crystal Drake",   unlocked:["Mini Crystal Drake","Shadow Fox","Pixel Slime","Lucky Cat"], color:"#4CD964" },
                ].map((row, i) => (
                  <div key={i} style={{ display: "flex", alignItems: "center", gap: 10, padding: "9px 12px", borderRadius: 8, border: "1px solid rgba(106,127,181,0.22)", background: "rgba(14,18,30,0.40)" }}>
                    {/* Slot */}
                    <div className="sv-slot" style={{ width: 44, height: 44, flexShrink: 0, borderColor: `${row.color}55`, background: "rgba(14,18,30,0.7)" }}>
                      <span style={{ fontSize: 18 }}>{row.icon}</span>
                    </div>
                    {/* Category info */}
                    <div style={{ flex: 1, minWidth: 0 }}>
                      <div style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: 3 }}>
                        <span style={{ ...pxFont, fontSize: 7, color: row.color }}>{row.cat}</span>
                        <span style={{ ...vtFont, fontSize: 12, color: "#6A7FB5" }}>{row.unlocked.length} unlocked</span>
                      </div>
                      <div style={{ ...vtFont, fontSize: 14, color: "#fff" }}>Equipped: <span style={{ color: row.color }}>{row.equipped}</span></div>
                      {/* Scrollable pill list of unlocked cosmetics */}
                      <div style={{ display: "flex", gap: 5, marginTop: 5, flexWrap: "wrap" }}>
                        {row.unlocked.map((name, j) => (
                          <div key={j} style={{ ...vtFont, fontSize: 12, padding: "2px 8px", borderRadius: 4, background: name === row.equipped ? `${row.color}22` : "rgba(58,64,96,0.5)", border: `1px solid ${name === row.equipped ? row.color + "66" : "rgba(106,127,181,0.22)"}`, color: name === row.equipped ? row.color : "#C7D0E0", cursor: "pointer" }}>
                            {name}
                          </div>
                        ))}
                      </div>
                    </div>
                    {/* Equip button */}
                    <Btn size="sm" variant="purple" style={{ flexShrink: 0, fontSize: 7 }}>CHANGE</Btn>
                  </div>
                ))}
              </div>
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── CRAFTING ── */}
      {subTab === "crafting" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <PanelHeader title="CRAFTING STATION" icon={<SvgIcon.Crafting size={14}/>} onClose={() => {}}/>
          <div style={{ padding: 20, display: "flex", gap: 20 }}>
            <div style={{ flex: 1 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#C7D0E0", marginBottom: 10 }}>RECIPES</div>
              <div className="sv-scroll" style={{ display: "flex", flexDirection: "column", gap: 6, maxHeight: 300, overflowY: "auto" }}>
                {[
                  { name:"Crystal Blade",  emoji:"⚔️",  mats:"Crystal x5 · Iron x3",    rarity:"epic",      learned:true },
                  { name:"Mana Potion",    emoji:"🧪",  mats:"Herb x3 · Water x1",       rarity:"common",    learned:true },
                  { name:"Shield of Dawn", emoji:"🛡️",  mats:"Crystal x8 · Wood x4",     rarity:"rare",      learned:true },
                  { name:"Arcane Staff",   emoji:"🪄",  mats:"Crystal x10 · Gem x2",     rarity:"epic",      learned:false},
                  { name:"Power Ring",     emoji:"💍",  mats:"Gold x3 · Crystal x2",     rarity:"rare",      learned:true },
                  { name:"Crystal Helm",   emoji:"⛑️",  mats:"Crystal x6 · Steel x4",   rarity:"rare",      learned:true },
                ].map((r, i) => (
                  <div key={i} style={{ display: "flex", alignItems: "center", gap: 12, padding: "10px 12px", borderRadius: 7, border: "1px solid rgba(106,127,181,0.22)", cursor: "pointer", background: i === 0 ? "rgba(79,140,255,0.10)" : "transparent", transition: "background .12s" }}
                    onMouseEnter={e => { (e.currentTarget as HTMLElement).style.background = "rgba(79,140,255,0.08)"; }}
                    onMouseLeave={e => { (e.currentTarget as HTMLElement).style.background = i === 0 ? "rgba(79,140,255,0.10)" : "transparent"; }}>
                    <span style={{ fontSize: 22 }}>{r.emoji}</span>
                    <div style={{ flex: 1 }}>
                      <div style={{ ...pxFont, fontSize: 7, color: r.learned ? "#fff" : "#6A7FB5" }}>{r.name}</div>
                      <div style={{ ...vtFont, fontSize: 12, color: "#C7D0E0", marginTop: 2 }}>{r.mats}</div>
                    </div>
                    <span style={{ ...pxFont, fontSize: 6, color: r.rarity === "epic" ? "#9B59B6" : r.rarity === "rare" ? "#4F8CFF" : "#C7D0E0" }}>{r.rarity.toUpperCase()}</span>
                    {!r.learned && <span style={{ ...pxFont, fontSize: 5, color: "#FF5A5A" }}>🔒</span>}
                  </div>
                ))}
              </div>
            </div>
            <div style={{ width: 180 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 10 }}>CRAFT: Crystal Blade</div>
              <div style={{ background: "rgba(18,22,38,0.85)", border: "1px solid rgba(106,127,181,0.35)", borderRadius: 8, padding: 14, textAlign: "center" }}>
                <span style={{ fontSize: 44 }}>⚔️</span>
                <div style={{ ...pxFont, fontSize: 7, color: "#9B59B6", margin: "8px 0 4px" }}>Crystal Blade</div>
                <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", marginBottom: 12 }}>EPIC WEAPON</div>
                <div style={{ display: "flex", flexDirection: "column", gap: 6, borderTop: "1px solid rgba(106,127,181,0.18)", paddingTop: 10 }}>
                  {[
                    { mat:"Crystal Shard", need:5, have:24, ok:true  },
                    { mat:"Iron Ore",      need:3, have:1,  ok:false },
                  ].map(m => (
                    <div key={m.mat} style={{ display: "flex", justifyContent: "space-between" }}>
                      <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>{m.mat}</span>
                      <span style={{ ...vtFont, fontSize: 13, color: m.ok ? "#4CD964" : "#FF5A5A" }}>{m.have}/{m.need} {m.ok ? "✓" : "✗"}</span>
                    </div>
                  ))}
                </div>
                <Btn variant="disabled" style={{ marginTop: 14, width: "100%" }}>CRAFT</Btn>
                <div style={{ ...vtFont, fontSize: 12, color: "#FF5A5A", marginTop: 6 }}>Missing: Iron Ore x2</div>
              </div>
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── NPC DIALOGUE ── */}
      {subTab === "npc" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <PanelHeader title="NPC DIALOGUE" onClose={() => {}}/>
          <div style={{ padding: 20, display: "flex", gap: 20 }}>
            <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 8, width: 100 }}>
              <div style={{ width: 80, height: 80, borderRadius: 10, background: "#141c2e", border: "1px solid rgba(77,225,255,0.35)", display: "flex", alignItems: "center", justifyContent: "center", fontSize: 40, boxShadow: "0 0 18px rgba(77,225,255,0.18)" }}>🧙‍♂️</div>
              <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", textAlign: "center" }}>ELDER MOROS</div>
              <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", textAlign: "center" }}>Crystal Keeper</div>
              <div style={{ ...vtFont, fontSize: 11, color: "#4CD964" }}>Friendly</div>
            </div>
            <div style={{ flex: 1 }}>
              <div style={{ background: "rgba(18,22,38,0.75)", border: "1px solid rgba(106,127,181,0.28)", borderRadius: 8, padding: "14px 16px", marginBottom: 14, minHeight: 90 }}>
                <div style={{ ...vtFont, fontSize: 17, color: "#fff", lineHeight: 1.5 }}>
                  "Traveler, the Crystal Nexus grows unstable. Ancient shards scatter across the realm —
                  will you gather them before darkness consumes the light? The fate of our world rests in your hands."
                </div>
              </div>
              <div style={{ display: "flex", flexDirection: "column", gap: 6 }}>
                {[
                  "Yes, I will gather the crystal shards.",
                  "Tell me more about the Crystal Nexus.",
                  "What reward will I receive?",
                  "I need more time to prepare.",
                  "Not now. Farewell, Elder.",
                ].map((opt, i) => (
                  <button key={i} style={{ ...vtFont, fontSize: 15, textAlign: "left", padding: "8px 14px", borderRadius: 6, cursor: "pointer", background: "rgba(79,140,255,0.05)", border: "1px solid rgba(79,140,255,0.22)", color: "#fff", transition: "all .12s", lineHeight: 1.3 }}
                    onMouseEnter={e => { (e.currentTarget as HTMLElement).style.background = "rgba(79,140,255,0.14)"; (e.currentTarget as HTMLElement).style.borderColor = "rgba(77,225,255,0.45)"; }}
                    onMouseLeave={e => { (e.currentTarget as HTMLElement).style.background = "rgba(79,140,255,0.05)"; (e.currentTarget as HTMLElement).style.borderColor = "rgba(79,140,255,0.22)"; }}>
                    <span style={{ color: "#4DE1FF", marginRight: 8 }}>[{i + 1}]</span>{opt}
                  </button>
                ))}
              </div>
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── SETTINGS ── */}
      {subTab === "settings" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <PanelHeader title="SETTINGS" icon={<span style={{ fontSize: 14 }}>⚙️</span>} onClose={() => {}} drag/>
          <div style={{ display: "flex", gap: 0, minHeight: 360 }}>
            {/* Left sidebar */}
            <div style={{ width: 152, borderRight: "1px solid rgba(106,127,181,0.20)", padding: "10px 8px", display: "flex", flexDirection: "column", gap: 2 }}>
              {["GRAPHICS","AUDIO","CONTROLS","GAMEPLAY","ACCESSIBILITY","UI SCALE"].map(cat => (
                <button key={cat} onClick={() => setSettingsTab(cat)}
                  style={{ ...pxFont, fontSize: 6, textAlign: "left", padding: "9px 12px", borderRadius: 5, cursor: "pointer", background: settingsTab === cat ? "rgba(79,140,255,0.18)" : "transparent", color: settingsTab === cat ? "#4DE1FF" : "#C7D0E0", borderLeft: settingsTab === cat ? "2px solid #4DE1FF" : "2px solid transparent", transition: "all .12s", display: "block", width: "100%" }}
                  onMouseEnter={e => { if (settingsTab !== cat) (e.currentTarget as HTMLElement).style.color = "#fff"; }}
                  onMouseLeave={e => { if (settingsTab !== cat) (e.currentTarget as HTMLElement).style.color = "#C7D0E0"; }}>
                  {cat}
                </button>
              ))}
            </div>
            {/* Right content */}
            <div style={{ flex: 1, padding: "14px 18px", overflowY: "auto", display: "flex", flexDirection: "column", gap: 8 }}>
              {settingsTab === "GRAPHICS" && [
                { label: "Resolution", control: <select className="sv-input" style={{ fontSize: 13 }}><option>1920×1080</option><option>1280×720</option><option>2560×1440</option></select> },
                { label: "Render Quality", control: <select className="sv-input" style={{ fontSize: 13 }}><option>Ultra</option><option>High</option><option>Medium</option><option>Low</option></select> },
                { label: "Shadow Quality", control: <select className="sv-input" style={{ fontSize: 13 }}><option>High</option><option>Medium</option><option>Off</option></select> },
                { label: "Particle Effects", control: <span style={{ ...vtFont, fontSize: 15, color: "#4CD964" }}>● ON</span> },
                { label: "VSync", control: <span style={{ ...vtFont, fontSize: 15, color: "#4CD964" }}>● ON</span> },
                { label: "FPS Limit", control: <select className="sv-input" style={{ fontSize: 13 }}><option>Unlimited</option><option>144</option><option>60</option></select> },
              ].map(row => (
                <div key={row.label} style={{ display: "flex", justifyContent: "space-between", alignItems: "center", padding: "8px 12px", borderRadius: 6, border: "1px solid rgba(106,127,181,0.16)", background: "rgba(14,18,30,0.35)" }}>
                  <span style={{ ...vtFont, fontSize: 15, color: "#C7D0E0" }}>{row.label}</span>
                  {row.control}
                </div>
              ))}
              {settingsTab === "AUDIO" && [
                { label: "Master Volume", val: 80 },
                { label: "Music Volume", val: 60 },
                { label: "SFX Volume", val: 90 },
                { label: "UI Sounds", val: 70 },
                { label: "Ambient", val: 50 },
              ].map(row => (
                <div key={row.label} style={{ display: "flex", justifyContent: "space-between", alignItems: "center", padding: "8px 12px", borderRadius: 6, border: "1px solid rgba(106,127,181,0.16)", background: "rgba(14,18,30,0.35)", gap: 12 }}>
                  <span style={{ ...vtFont, fontSize: 15, color: "#C7D0E0", minWidth: 120 }}>{row.label}</span>
                  <div style={{ flex: 1, height: 6, background: "rgba(106,127,181,0.25)", borderRadius: 3, overflow: "hidden" }}>
                    <div style={{ width: `${row.val}%`, height: "100%", background: "#4F8CFF", borderRadius: 3 }}/>
                  </div>
                  <span style={{ ...monoFont, fontSize: 13, color: "#4DE1FF", minWidth: 36, textAlign: "right" }}>{row.val}%</span>
                </div>
              ))}
              {settingsTab === "CONTROLS" && [
                { action: "Move Up",       key: "W" },
                { action: "Move Down",     key: "S" },
                { action: "Move Left",     key: "A" },
                { action: "Move Right",    key: "D" },
                { action: "Jump",          key: "SPACE" },
                { action: "Interact",      key: "E" },
                { action: "Open Inventory",key: "TAB" },
                { action: "Chat",          key: "ENTER" },
              ].map(row => (
                <div key={row.action} style={{ display: "flex", justifyContent: "space-between", alignItems: "center", padding: "8px 12px", borderRadius: 6, border: "1px solid rgba(106,127,181,0.16)", background: "rgba(14,18,30,0.35)" }}>
                  <span style={{ ...vtFont, fontSize: 15, color: "#C7D0E0" }}>{row.action}</span>
                  <span style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", background: "rgba(79,140,255,0.18)", border: "1px solid rgba(79,140,255,0.40)", padding: "4px 10px", borderRadius: 4 }}>{row.key}</span>
                </div>
              ))}
              {settingsTab === "GAMEPLAY" && [
                { label: "Auto Loot",        on: true },
                { label: "PvP Mode",         on: false },
                { label: "Tutorial Hints",   on: true },
                { label: "Damage Numbers",   on: true },
                { label: "Show Player Names",on: true },
                { label: "Combat Log",       on: false },
              ].map(row => (
                <div key={row.label} style={{ display: "flex", justifyContent: "space-between", alignItems: "center", padding: "8px 12px", borderRadius: 6, border: "1px solid rgba(106,127,181,0.16)", background: "rgba(14,18,30,0.35)" }}>
                  <span style={{ ...vtFont, fontSize: 15, color: "#C7D0E0" }}>{row.label}</span>
                  <span style={{ ...vtFont, fontSize: 15, color: row.on ? "#4CD964" : "#FF5A5A" }}>{row.on ? "● ON" : "○ OFF"}</span>
                </div>
              ))}
              {settingsTab === "ACCESSIBILITY" && [
                { label: "Colorblind Mode",  on: false },
                { label: "Large Text",       on: false },
                { label: "Reduce Motion",    on: false },
                { label: "Screen Reader",    on: false },
                { label: "High Contrast",    on: false },
              ].map(row => (
                <div key={row.label} style={{ display: "flex", justifyContent: "space-between", alignItems: "center", padding: "8px 12px", borderRadius: 6, border: "1px solid rgba(106,127,181,0.16)", background: "rgba(14,18,30,0.35)" }}>
                  <span style={{ ...vtFont, fontSize: 15, color: "#C7D0E0" }}>{row.label}</span>
                  <span style={{ ...vtFont, fontSize: 15, color: row.on ? "#4CD964" : "#FF5A5A" }}>{row.on ? "● ON" : "○ OFF"}</span>
                </div>
              ))}
              {settingsTab === "UI SCALE" && (
                <div>
                  <div style={{ ...vtFont, fontSize: 15, color: "#C7D0E0", marginBottom: 12 }}>Select interface scale. Changes apply immediately.</div>
                  <div style={{ display: "flex", gap: 10, flexWrap: "wrap" }}>
                    {[75, 100, 125, 150].map(scale => (
                      <div key={scale} style={{ flex: "1 1 120px", padding: "18px 10px", borderRadius: 8, border: scale === 100 ? "1px solid #4DE1FF" : "1px solid rgba(106,127,181,0.30)", background: scale === 100 ? "rgba(77,225,255,0.08)" : "rgba(14,18,30,0.55)", cursor: "pointer", textAlign: "center" }}>
                        <div style={{ ...pxFont, fontSize: 12, color: scale === 100 ? "#4DE1FF" : "#C7D0E0", marginBottom: 4 }}>{scale}%</div>
                        <div style={{ ...vtFont, fontSize: 13, color: "#6A7FB5" }}>{scale === 75 ? "Compact" : scale === 100 ? "Default" : scale === 125 ? "Comfortable" : "Large"}</div>
                        {scale === 100 && <div style={{ ...pxFont, fontSize: 5, color: "#4CD964", marginTop: 6 }}>✓ ACTIVE</div>}
                      </div>
                    ))}
                  </div>
                </div>
              )}
              <div style={{ marginTop: 8, display: "flex", gap: 8 }}>
                <Btn style={{ flex: 1 }}>SAVE CHANGES</Btn>
                <Btn variant="secondary" style={{ flex: 1 }}>RESET TO DEFAULTS</Btn>
              </div>
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── STORAGE ── */}
      {subTab === "storage" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <div className="sv-panel-header sv-panel-drag">
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <span style={{ fontSize: 14 }}>📦</span>
              <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>WORLD STORAGE</span>
              <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>Crystal-01 / Home Base</span>
            </div>
            <div style={{ display: "flex", gap: 10, alignItems: "center" }}>
              <span style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>48/100 slots</span>
              <button className="sv-btn sv-btn-disabled" style={{ width: 24, height: 24, padding: 0, fontSize: 9 }}>✕</button>
            </div>
          </div>
          <div style={{ padding: 16, display: "flex", gap: 16 }}>
            {/* Chest selector */}
            <div style={{ width: 130, flexShrink: 0 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 8 }}>CONTAINERS</div>
              {[
                { label:"Home Chest",   icon:"📦", used:24, cap:50 },
                { label:"Material Box", icon:"🪨", used:18, cap:30 },
                { label:"Equipment Safe",icon:"⚔️",used:6,  cap:20 },
              ].map((c, i) => (
                <div key={i} style={{ display: "flex", alignItems: "center", gap: 8, padding: "7px 10px", borderRadius: 6, marginBottom: 4, background: i === 0 ? "rgba(79,140,255,0.14)" : "rgba(14,18,30,0.5)", border: `1px solid ${i === 0 ? "rgba(79,140,255,0.40)" : "rgba(106,127,181,0.20)"}`, cursor: "pointer" }}>
                  <span style={{ fontSize: 16 }}>{c.icon}</span>
                  <div style={{ flex: 1, minWidth: 0 }}>
                    <div style={{ ...pxFont, fontSize: 6, color: i === 0 ? "#4DE1FF" : "#C7D0E0", whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis" }}>{c.label}</div>
                    <div style={{ ...monoFont, fontSize: 8, color: "#6A7FB5", marginTop: 1 }}>{c.used}/{c.cap}</div>
                  </div>
                </div>
              ))}
              <Btn size="sm" style={{ width: "100%", marginTop: 6 }}>+ ADD CHEST</Btn>
            </div>
            {/* Grid */}
            <div style={{ flex: 1 }}>
              <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 6 }}>
                <span style={{ ...pxFont, fontSize: 7, color: "#C7D0E0" }}>HOME CHEST</span>
                <div style={{ display: "flex", gap: 6 }}>
                  <Btn size="sm">DEPOSIT ALL</Btn>
                  <Btn size="sm" variant="purple">WITHDRAW</Btn>
                  <Btn size="sm" variant="success">TRANSFER →</Btn>
                </div>
              </div>
              <div style={{ display: "flex", gap: 4, marginBottom: 8, alignItems: "center" }}>
                <input className="sv-input" placeholder="Search storage..." value={storageSearch} onChange={e => setStorageSearch(e.target.value)} style={{ flex: 1, padding: "3px 8px", fontSize: 13 }}/>
                {(["default","name"] as const).map(s => (
                  <button key={s} onClick={() => setStorageSort(s)} style={{ ...vtFont, fontSize: 13, padding: "3px 8px", borderRadius: 4, cursor: "pointer", background: storageSort === s ? "rgba(77,225,255,0.14)" : "transparent", color: storageSort === s ? "#4DE1FF" : "#C7D0E0", border: storageSort === s ? "1px solid rgba(77,225,255,0.40)" : "1px solid transparent", transition: "all .12s", textTransform: "uppercase" }}>
                    {s}
                  </button>
                ))}
              </div>
              {(() => {
                let storageItems = (INV_ITEMS.concat([null,null,null,null,null,null]).slice(0, 24).filter(Boolean) as NonNullable<typeof INV_ITEMS[0]>[]);
                if (storageSearch) storageItems = storageItems.filter(it => it.name.toLowerCase().includes(storageSearch.toLowerCase()));
                if (storageSort === "name") storageItems = [...storageItems].sort((a, b) => a.name.localeCompare(b.name));
                const displayStorage: (typeof INV_ITEMS[0])[] = [...storageItems];
                while (displayStorage.length < 24) displayStorage.push(null);
                return (
              <div style={{ display: "grid", gridTemplateColumns: "repeat(8, 48px)", gap: 4 }}>
                {displayStorage.map((item, i) => (
                  <div key={i} className={`sv-slot ${item ? RARITY_CLS[item.rarity] : ""}`} style={{ width: 48, height: 48 }}>
                    {item && <span style={{ fontSize: 18 }}>{item.emoji}</span>}
                    {item && item.qty > 1 && <span style={{ ...pxFont, position: "absolute", bottom: 1, right: 2, fontSize: 5, color: "#FFD700" }}>{item.qty > 99 ? "99+" : item.qty}</span>}
                  </div>
                ))}
              </div>
                );
              })()}
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── TRADING ── */}
      {subTab === "trading" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <div className="sv-panel-header sv-panel-drag">
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <span style={{ fontSize: 14 }}>🔄</span>
              <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>TRADE</span>
            </div>
            <button className="sv-btn sv-btn-danger" style={{ fontSize: 7, padding: "4px 10px" }}>CANCEL</button>
          </div>
          <div style={{ padding: 16, display: "flex", gap: 12 }}>
            {/* Your offer */}
            {[
              { label: "YOUR OFFER", player: "CrystalMage", color: "#4F8CFF", accepted: true },
              { label: "THEIR OFFER", player: "PixelMage",  color: "#6C5CE7", accepted: false },
            ].map((side, si) => (
              <div key={si} style={{ flex: 1 }}>
                <div style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: 8 }}>
                  <span style={{ ...pxFont, fontSize: 7, color: side.color }}>{side.label}</span>
                  <span style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>— {side.player}</span>
                  {side.accepted && <span style={{ ...pxFont, fontSize: 5, color: "#4CD964", background: "rgba(76,217,100,0.14)", padding: "1px 5px", borderRadius: 3 }}>✓ READY</span>}
                </div>
                <div style={{ display: "grid", gridTemplateColumns: "repeat(4, 54px)", gap: 4, padding: 10, background: "rgba(14,18,30,0.55)", border: `1px solid ${side.color}33`, borderRadius: 8 }}>
                  {(si === 0 ? [INV_ITEMS[0], INV_ITEMS[1], null, null, null, null, null, null] : [INV_ITEMS[2], INV_ITEMS[3], null, null, null, null, null, null]).map((item, i) => (
                    <div key={i} className={`sv-slot ${item ? RARITY_CLS[item.rarity] : ""}`} style={{ width: 52, height: 52 }}>
                      {item && <span style={{ fontSize: 20 }}>{item.emoji}</span>}
                      {item && item.qty > 1 && <span style={{ ...pxFont, position: "absolute", bottom: 1, right: 2, fontSize: 5, color: "#FFD700" }}>{item.qty > 99 ? "99+" : item.qty}</span>}
                    </div>
                  ))}
                </div>
                {/* Gold offer */}
                <div style={{ display: "flex", alignItems: "center", gap: 8, marginTop: 8, padding: "7px 12px", background: "rgba(14,18,30,0.55)", border: "1px solid rgba(255,215,0,0.25)", borderRadius: 7 }}>
                  <span style={{ fontSize: 16 }}>🪙</span>
                  <span style={{ ...vtFont, fontSize: 15, color: "#FFD700" }}>{si === 0 ? "500" : "0"} Gold</span>
                </div>
              </div>
            ))}
          </div>
          <div style={{ padding: "10px 16px", borderTop: "1px solid rgba(106,127,181,0.20)", display: "flex", gap: 8, justifyContent: "center" }}>
            <Btn variant="success" style={{ minWidth: 160 }}>✓ ACCEPT TRADE</Btn>
            <Btn variant="danger">DECLINE</Btn>
          </div>
        </CrystalPanel>
      )}

      {/* ── MARKETPLACE ── */}
      {subTab === "marketplace" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <div className="sv-panel-header sv-panel-drag">
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <SvgIcon.Marketplace size={14} color="#FFD700"/>
              <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>MARKETPLACE</span>
            </div>
            <div style={{ display: "flex", gap: 8, alignItems: "center" }}>
              <span style={{ ...vtFont, fontSize: 14, color: "#FFD700" }}>🪙 4,280</span>
              <button className="sv-btn sv-btn-disabled" style={{ width: 24, height: 24, padding: 0, fontSize: 9 }}>✕</button>
            </div>
          </div>
          <div style={{ padding: 12 }}>
            {/* Coins-only banner */}
            <div style={{ display: "flex", alignItems: "center", gap: 10, padding: "7px 12px", marginBottom: 12, background: "rgba(255,215,0,0.07)", border: "1px solid rgba(255,215,0,0.28)", borderRadius: 7 }}>
              <span style={{ fontSize: 16 }}>🪙</span>
              <span style={{ ...vtFont, fontSize: 15, color: "#FFD700" }}>The Marketplace uses <strong>Coins only</strong>. Gems are never accepted here.</span>
              <span style={{ marginLeft: "auto", ...pxFont, fontSize: 5, color: "#4CD964", background: "rgba(76,217,100,0.12)", padding: "2px 7px", borderRadius: 4 }}>COINS ONLY</span>
            </div>
            {/* Search + filter */}
            <div style={{ display: "flex", gap: 8, marginBottom: 12 }}>
              <div style={{ flex: 1, background: "rgba(14,18,30,0.70)", border: "1px solid rgba(106,127,181,0.35)", borderRadius: 6, padding: "6px 12px", display: "flex", alignItems: "center", gap: 6 }}>
                <span style={{ ...vtFont, fontSize: 14, color: "#6A7FB5" }}>🔍</span>
                <span style={{ ...vtFont, fontSize: 15, color: "#6A7FB5" }}>Search items...</span>
              </div>
              {["ALL","WEAPONS","ARMOR","MATS","COSMETICS"].map((f, i) => (
                <button key={f} style={{ ...pxFont, fontSize: 6, padding: "6px 10px", borderRadius: 6, cursor: "pointer", background: i === 0 ? "rgba(79,140,255,0.18)" : "rgba(44,49,69,0.55)", color: i === 0 ? "#4DE1FF" : "#C7D0E0", border: i === 0 ? "1px solid rgba(79,140,255,0.45)" : "1px solid rgba(106,127,181,0.22)" }}>
                  {f}
                </button>
              ))}
            </div>
            {/* Listings */}
            <div style={{ display: "flex", flexDirection: "column", gap: 5 }}>
              {[
                { emoji:"🔮", name:"Arcane Orb",       rarity:"legendary", seller:"MysticArcher",  price:2800, qty:1  },
                { emoji:"⚔️", name:"Crystal Blade",     rarity:"epic",      seller:"SteelWarden",  price:1200, qty:1  },
                { emoji:"💎", name:"Crystal Shard x10", rarity:"rare",      seller:"GemHunter",    price:350,  qty:10 },
                { emoji:"🧪", name:"Mana Potion x5",    rarity:"common",    seller:"AlchemyMike",  price:80,   qty:5  },
                { emoji:"🪖", name:"Crystal Helm",      rarity:"rare",      seller:"CrystalKnight",price:620,  qty:1  },
                { emoji:"🦋", name:"Crystal Wings",     rarity:"epic",      seller:"CosmicWitch",  price:3400, qty:1  },
              ].map((l, i) => (
                <div key={i} style={{ display: "flex", alignItems: "center", gap: 12, padding: "8px 12px", borderRadius: 7, border: "1px solid rgba(106,127,181,0.22)", background: i === 0 ? "rgba(79,140,255,0.06)" : "transparent", transition: "background .12s", cursor: "pointer" }}
                  onMouseEnter={e => { (e.currentTarget as HTMLElement).style.background = "rgba(79,140,255,0.08)"; }}
                  onMouseLeave={e => { (e.currentTarget as HTMLElement).style.background = i === 0 ? "rgba(79,140,255,0.06)" : "transparent"; }}>
                  <div className={`sv-slot ${RARITY_CLS[l.rarity]}`} style={{ width: 42, height: 42, flexShrink: 0 }}>
                    <span style={{ fontSize: 18 }}>{l.emoji}</span>
                  </div>
                  <div style={{ flex: 1 }}>
                    <div style={{ display: "flex", gap: 8, alignItems: "center" }}>
                      <span style={{ ...pxFont, fontSize: 7, color: "#fff" }}>{l.name}</span>
                      <span style={{ ...pxFont, fontSize: 5, color: l.rarity === "legendary" ? "#FFD700" : l.rarity === "epic" ? "#9B59B6" : l.rarity === "rare" ? "#4F8CFF" : "#C7D0E0" }}>{l.rarity.toUpperCase()}</span>
                    </div>
                    <span style={{ ...vtFont, fontSize: 13, color: "#6A7FB5" }}>Seller: {l.seller}</span>
                  </div>
                  <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
                    <span style={{ ...monoFont, fontSize: 11, color: "#FFD700" }}>🪙 {l.price.toLocaleString()}</span>
                    <Btn size="sm" variant="success" style={{ fontSize: 7 }}>BUY</Btn>
                  </div>
                </div>
              ))}
            </div>
            <div style={{ marginTop: 10, display: "flex", justifyContent: "flex-end", gap: 8 }}>
              <Btn size="sm" variant="purple">MY LISTINGS</Btn>
              <Btn size="sm">+ SELL ITEM</Btn>
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── FRIENDS ── */}
      {subTab === "friends" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <div className="sv-panel-header sv-panel-drag">
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <SvgIcon.Friends size={14} color="#4CD964"/>
              <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>FRIENDS</span>
              <span style={{ ...vtFont, fontSize: 14, color: "#4CD964" }}>7 online</span>
            </div>
            <div style={{ display: "flex", gap: 8 }}>
              <Btn size="sm">+ ADD FRIEND</Btn>
              <button className="sv-btn sv-btn-disabled" style={{ width: 24, height: 24, padding: 0, fontSize: 9 }}>✕</button>
            </div>
          </div>
          <div style={{ padding: "0 0 16px" }}>
            {/* Search */}
            <div style={{ padding: "8px 16px", borderBottom: "1px solid rgba(106,127,181,0.18)" }}>
              <div style={{ background: "rgba(14,18,30,0.55)", border: "1px solid rgba(106,127,181,0.28)", borderRadius: 5, padding: "5px 10px", display: "flex", gap: 6, alignItems: "center" }}>
                <span style={{ ...vtFont, fontSize: 14, color: "#6A7FB5" }}>🔍</span>
                <span style={{ ...vtFont, fontSize: 14, color: "#6A7FB5" }}>Search friends...</span>
              </div>
            </div>
            {/* Online */}
            <div style={{ padding: "10px 16px 4px" }}>
              <span style={{ ...pxFont, fontSize: 6, color: "#4CD964" }}>ONLINE — 7</span>
            </div>
            {[
              { name:"CrystalKnight", lv:58, world:"Crystal-01", status:"In Dungeon",   col:"#FFD700" },
              { name:"PixelMage",     lv:42, world:"Void-Gate",  status:"Building",     col:"#6C5CE7" },
              { name:"ArcaneWitch",   lv:35, world:"Crystal-01", status:"In World",     col:"#4DE1FF" },
              { name:"ShadowRogue",   lv:61, world:"Shadow-Keep",status:"Boss Fight",   col:"#FF5A5A" },
            ].map((f, i) => (
              <div key={i} style={{ display: "flex", alignItems: "center", gap: 10, padding: "8px 16px", borderRadius: 6, margin: "2px 8px", background: "transparent", transition: "background .12s", cursor: "pointer" }}
                onMouseEnter={e => { (e.currentTarget as HTMLElement).style.background = "rgba(79,140,255,0.07)"; }}
                onMouseLeave={e => { (e.currentTarget as HTMLElement).style.background = "transparent"; }}>
                <div style={{ width: 38, height: 38, borderRadius: 8, background: "rgba(14,18,30,0.8)", border: `1px solid ${f.col}44`, display: "flex", alignItems: "center", justifyContent: "center", fontSize: 18, flexShrink: 0 }}>🧙</div>
                <div style={{ flex: 1 }}>
                  <div style={{ display: "flex", gap: 6, alignItems: "center" }}>
                    <span style={{ ...pxFont, fontSize: 7, color: "#fff" }}>{f.name}</span>
                    <span style={{ ...monoFont, fontSize: 8, color: "#4DE1FF" }}>Lv.{f.lv}</span>
                  </div>
                  <div style={{ display: "flex", gap: 6, alignItems: "center", marginTop: 2 }}>
                    <span style={{ width: 6, height: 6, borderRadius: "50%", background: "#4CD964", flexShrink: 0, display: "inline-block" }}/>
                    <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>{f.status} · {f.world}</span>
                  </div>
                </div>
                <div style={{ display: "flex", gap: 5 }}>
                  <button title="Whisper" style={{ background: "rgba(79,140,255,0.10)", border: "1px solid rgba(79,140,255,0.30)", borderRadius: 4, padding: "3px 7px", cursor: "pointer", fontSize: 11 }}>💬</button>
                  <button title="Visit World" style={{ background: "rgba(76,217,100,0.10)", border: "1px solid rgba(76,217,100,0.30)", borderRadius: 4, padding: "3px 7px", cursor: "pointer", fontSize: 11 }}>🌐</button>
                </div>
              </div>
            ))}
            <div style={{ padding: "10px 16px 4px", marginTop: 4, borderTop: "1px solid rgba(106,127,181,0.15)" }}>
              <span style={{ ...pxFont, fontSize: 6, color: "#6A7FB5" }}>OFFLINE — 3</span>
            </div>
            {["GhostRider","StoneWarden","IronFist"].map((n, i) => (
              <div key={i} style={{ display: "flex", alignItems: "center", gap: 10, padding: "7px 16px", margin: "2px 8px", borderRadius: 6 }}>
                <div style={{ width: 38, height: 38, borderRadius: 8, background: "rgba(14,18,30,0.5)", border: "1px solid rgba(106,127,181,0.18)", display: "flex", alignItems: "center", justifyContent: "center", fontSize: 18, opacity: 0.5, flexShrink: 0 }}>🧙</div>
                <div style={{ flex: 1 }}>
                  <span style={{ ...pxFont, fontSize: 7, color: "#6A7FB5" }}>{n}</span>
                  <div style={{ display: "flex", gap: 5, alignItems: "center", marginTop: 2 }}>
                    <span style={{ width: 6, height: 6, borderRadius: "50%", background: "#6A7FB5", flexShrink: 0, display: "inline-block" }}/>
                    <span style={{ ...vtFont, fontSize: 13, color: "#6A7FB5" }}>Offline</span>
                  </div>
                </div>
              </div>
            ))}
          </div>
        </CrystalPanel>
      )}

      {/* ── GUILD ── */}
      {subTab === "guild" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <div className="sv-panel-header sv-panel-drag">
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <SvgIcon.Guild size={14} color="#FFD700"/>
              <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>CRYSTAL VANGUARD</span>
              <span style={{ ...vtFont, fontSize: 13, color: "#FFD54A" }}>Rank: Elite</span>
            </div>
            <button className="sv-btn sv-btn-disabled" style={{ width: 24, height: 24, padding: 0, fontSize: 9 }}>✕</button>
          </div>
          <div style={{ padding: 16, display: "flex", gap: 16 }}>
            {/* Guild info */}
            <div style={{ width: 180, flexShrink: 0 }}>
              <div style={{ textAlign: "center", padding: 16, background: "rgba(14,18,30,0.60)", border: "1px solid rgba(255,215,0,0.25)", borderRadius: 10, marginBottom: 12 }}>
                <div style={{ fontSize: 40, marginBottom: 6 }}>🏰</div>
                <div style={{ ...pxFont, fontSize: 8, color: "#FFD700", marginBottom: 4 }}>CRYSTAL VANGUARD</div>
                <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", marginBottom: 8 }}>"United by Crystal, Unbroken by Shadow"</div>
                <div style={{ display: "flex", justifyContent: "center", gap: 12 }}>
                  <div style={{ textAlign: "center" }}>
                    <div style={{ ...monoFont, fontSize: 11, color: "#4DE1FF" }}>38</div>
                    <div style={{ ...vtFont, fontSize: 11, color: "#6A7FB5" }}>Members</div>
                  </div>
                  <div style={{ textAlign: "center" }}>
                    <div style={{ ...monoFont, fontSize: 11, color: "#FFD700" }}>Lv.12</div>
                    <div style={{ ...vtFont, fontSize: 11, color: "#6A7FB5" }}>Guild Lv</div>
                  </div>
                </div>
              </div>
              <div style={{ display: "flex", flexDirection: "column", gap: 5 }}>
                <div style={{ ...pxFont, fontSize: 6, color: "#4DE1FF", marginBottom: 4 }}>GUILD XP</div>
                <Bar value={68} max={100} color="#FFD700"/>
                <span style={{ ...monoFont, fontSize: 8, color: "#C7D0E0" }}>68,200 / 100,000</span>
                <Btn size="sm" style={{ marginTop: 6 }}>GUILD BANK</Btn>
                <Btn size="sm" variant="purple">GUILD HALL</Btn>
              </div>
            </div>
            {/* Member list */}
            <div style={{ flex: 1 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 8 }}>MEMBERS</div>
              <div className="sv-scroll" style={{ display: "flex", flexDirection: "column", gap: 4, maxHeight: 300, overflowY: "auto" }}>
                {[
                  { name:"CrystalMage",  lv:42, rank:"Member",    online:true,  contrib:4200 },
                  { name:"CrystalKnight",lv:58, rank:"Officer",   online:true,  contrib:8800 },
                  { name:"PixelMage",    lv:42, rank:"Member",    online:true,  contrib:3100 },
                  { name:"GuildMaster",  lv:75, rank:"Guild Master",online:true,contrib:24000},
                  { name:"ArcaneWitch",  lv:35, rank:"Recruit",   online:true,  contrib:900  },
                  { name:"GhostRider",   lv:50, rank:"Member",    online:false, contrib:5200 },
                  { name:"StoneWarden",  lv:44, rank:"Member",    online:false, contrib:3700 },
                ].map((m, i) => (
                  <div key={i} style={{ display: "flex", alignItems: "center", gap: 10, padding: "7px 10px", borderRadius: 6, background: "rgba(14,18,30,0.45)", border: "1px solid rgba(106,127,181,0.18)" }}>
                    <span style={{ width: 7, height: 7, borderRadius: "50%", background: m.online ? "#4CD964" : "#6A7FB5", flexShrink: 0, display: "inline-block" }}/>
                    <div style={{ flex: 1 }}>
                      <div style={{ display: "flex", gap: 6, alignItems: "center" }}>
                        <span style={{ ...pxFont, fontSize: 7, color: "#fff" }}>{m.name}</span>
                        <span style={{ ...monoFont, fontSize: 8, color: "#4DE1FF" }}>Lv.{m.lv}</span>
                      </div>
                      <span style={{ ...vtFont, fontSize: 12, color: "#6A7FB5" }}>{m.rank}</span>
                    </div>
                    <div style={{ textAlign: "right" }}>
                      <span style={{ ...monoFont, fontSize: 9, color: "#FFD54A" }}>🪙 {m.contrib.toLocaleString()}</span>
                      <div style={{ ...vtFont, fontSize: 11, color: "#6A7FB5" }}>contrib</div>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── MAIL ── */}
      {subTab === "mail" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <div className="sv-panel-header sv-panel-drag">
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <SvgIcon.Mail size={14} color="#4F8CFF"/>
              <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>MAIL</span>
              <span style={{ ...vtFont, fontSize: 14, color: "#FF5A5A" }}>3 unread</span>
            </div>
            <div style={{ display: "flex", gap: 8 }}>
              <Btn size="sm" variant="purple">✉ COMPOSE</Btn>
              <button className="sv-btn sv-btn-disabled" style={{ width: 24, height: 24, padding: 0, fontSize: 9 }}>✕</button>
            </div>
          </div>
          <div style={{ padding: 0, display: "flex", height: 340 }}>
            {/* Inbox list */}
            <div style={{ width: 260, borderRight: "1px solid rgba(106,127,181,0.20)", overflowY: "auto", flexShrink: 0 }} className="sv-scroll">
              <div style={{ padding: "8px 12px", borderBottom: "1px solid rgba(106,127,181,0.15)" }}>
                <span style={{ ...pxFont, fontSize: 6, color: "#4DE1FF" }}>INBOX</span>
              </div>
              {[
                { from:"System",       subject:"Welcome to StrixVerse!",      time:"2h ago",  unread:false, icon:"⚙️"  },
                { from:"CrystalKnight",subject:"Trade offer for Crystal Blade",time:"4h ago", unread:true,  icon:"🧙"  },
                { from:"Market Bot",   subject:"Your Arcane Orb sold!",        time:"6h ago", unread:true,  icon:"🛒"  },
                { from:"Guild Master", subject:"Guild event this Saturday",     time:"1d ago", unread:true,  icon:"🏰"  },
                { from:"ArcaneWitch",  subject:"GG on that dungeon run!",      time:"2d ago", unread:false, icon:"🧙"  },
              ].map((m, i) => (
                <div key={i} style={{ display: "flex", gap: 10, padding: "9px 12px", cursor: "pointer", background: i === 2 ? "rgba(79,140,255,0.10)" : "transparent", borderBottom: "1px solid rgba(106,127,181,0.10)", borderLeft: m.unread ? "3px solid #4F8CFF" : "3px solid transparent", transition: "background .12s" }}
                  onMouseEnter={e => { (e.currentTarget as HTMLElement).style.background = "rgba(79,140,255,0.07)"; }}
                  onMouseLeave={e => { (e.currentTarget as HTMLElement).style.background = i === 2 ? "rgba(79,140,255,0.10)" : "transparent"; }}>
                  <span style={{ fontSize: 16, flexShrink: 0 }}>{m.icon}</span>
                  <div style={{ flex: 1, minWidth: 0 }}>
                    <div style={{ display: "flex", justifyContent: "space-between" }}>
                      <span style={{ ...pxFont, fontSize: 6, color: m.unread ? "#fff" : "#C7D0E0", whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis" }}>{m.from}</span>
                      <span style={{ ...monoFont, fontSize: 8, color: "#6A7FB5", flexShrink: 0, marginLeft: 4 }}>{m.time}</span>
                    </div>
                    <div style={{ ...vtFont, fontSize: 13, color: "#6A7FB5", whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis", marginTop: 2 }}>{m.subject}</div>
                  </div>
                </div>
              ))}
            </div>
            {/* Mail reader */}
            <div style={{ flex: 1, display: "flex", flexDirection: "column", padding: 16 }}>
              <div style={{ marginBottom: 12, paddingBottom: 12, borderBottom: "1px solid rgba(106,127,181,0.20)" }}>
                <div style={{ ...pxFont, fontSize: 9, color: "#fff", marginBottom: 6 }}>Your Arcane Orb sold!</div>
                <div style={{ display: "flex", gap: 10, flexWrap: "wrap" }}>
                  <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>From: <span style={{ color: "#4DE1FF" }}>Market Bot</span></span>
                  <span style={{ ...vtFont, fontSize: 13, color: "#6A7FB5" }}>6 hours ago</span>
                </div>
              </div>
              <div style={{ flex: 1, ...vtFont, fontSize: 16, color: "#C7D0E0", lineHeight: 1.6 }}>
                Congratulations! Your listing for <span style={{ color: "#FFD700" }}>Arcane Orb x1 (Legendary)</span> sold for{" "}
                <span style={{ color: "#FFD700" }}>🪙 2,800 Gold</span>.
                <br/><br/>
                The gold has been deposited directly into your account. Thank you for using the StrixVerse Marketplace!
              </div>
              <div style={{ display: "flex", gap: 8, marginTop: 12 }}>
                <Btn size="sm">REPLY</Btn>
                <Btn size="sm" variant="danger">DELETE</Btn>
              </div>
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── QUEST LOG ── */}
      {subTab === "quest" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <div className="sv-panel-header sv-panel-drag">
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <SvgIcon.Quests size={14} color="#FFD54A"/>
              <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>QUEST LOG</span>
            </div>
            <button className="sv-btn sv-btn-disabled" style={{ width: 24, height: 24, padding: 0, fontSize: 9 }}>✕</button>
          </div>
          <div style={{ padding: 0, display: "flex", height: 380 }}>
            {/* Quest list */}
            <div style={{ width: 260, borderRight: "1px solid rgba(106,127,181,0.20)", overflowY: "auto", flexShrink: 0 }} className="sv-scroll">
              {[
                { cat:"ACTIVE" },
                { name:"The Crystal Hunt",     type:"MAIN",    prog:60, cat: null },
                { name:"Stone Golem Slayer",    type:"MAIN",    prog:0,  cat: null },
                { name:"Collect Rare Herbs",    type:"SIDE",    prog:75, cat: null },
                { name:"Deliver the Package",   type:"SIDE",    prog:100,cat: null },
                { cat:"COMPLETED" },
                { name:"A New Beginning",       type:"MAIN",    prog:100,cat: null, done:true },
                { name:"First Crystal Shard",   type:"SIDE",    prog:100,cat: null, done:true },
              ].map((q, i) => {
                if ("cat" in q && q.cat) return (
                  <div key={i} style={{ padding: "8px 12px 4px", ...pxFont, fontSize: 6, color: q.cat === "ACTIVE" ? "#4DE1FF" : "#6A7FB5", borderTop: i > 0 ? "1px solid rgba(106,127,181,0.15)" : undefined, marginTop: i > 0 ? 4 : 0 }}>{q.cat}</div>
                );
                const item = q as { name:string; type:string; prog:number; done?:boolean };
                return (
                  <div key={i} style={{ display: "flex", alignItems: "center", gap: 8, padding: "8px 12px", cursor: "pointer", background: i === 1 ? "rgba(79,140,255,0.10)" : "transparent", borderBottom: "1px solid rgba(106,127,181,0.10)", opacity: item.done ? 0.55 : 1, transition: "background .12s" }}
                    onMouseEnter={e => { (e.currentTarget as HTMLElement).style.background = "rgba(79,140,255,0.07)"; }}
                    onMouseLeave={e => { (e.currentTarget as HTMLElement).style.background = i === 1 ? "rgba(79,140,255,0.10)" : "transparent"; }}>
                    <span style={{ ...pxFont, fontSize: 5, color: item.type === "MAIN" ? "#FFD54A" : "#4F8CFF", background: item.type === "MAIN" ? "rgba(255,213,74,0.12)" : "rgba(79,140,255,0.12)", padding: "1px 5px", borderRadius: 3, flexShrink: 0 }}>{item.type}</span>
                    <span style={{ ...vtFont, fontSize: 14, color: item.done ? "#6A7FB5" : "#fff", flex: 1 }}>{item.name}</span>
                    {item.done && <span style={{ fontSize: 10 }}>✓</span>}
                  </div>
                );
              })}
            </div>
            {/* Quest detail */}
            <div style={{ flex: 1, padding: 16, display: "flex", flexDirection: "column", gap: 12, overflowY: "auto" }} className="sv-scroll">
              <div>
                <div style={{ display: "flex", gap: 8, alignItems: "center", marginBottom: 6 }}>
                  <span style={{ ...pxFont, fontSize: 7, color: "#FFD54A", background: "rgba(255,213,74,0.12)", padding: "2px 7px", borderRadius: 4 }}>MAIN QUEST</span>
                  <span style={{ ...pxFont, fontSize: 7, color: "#4CD964", background: "rgba(76,217,100,0.12)", padding: "2px 7px", borderRadius: 4 }}>IN PROGRESS</span>
                </div>
                <div style={{ ...pxFont, fontSize: 10, color: "#fff", marginBottom: 8 }}>The Crystal Hunt</div>
                <div style={{ ...vtFont, fontSize: 16, color: "#C7D0E0", lineHeight: 1.55 }}>
                  Elder Moros has asked you to gather scattered Crystal Shards before darkness consumes the realm. Venture deep into the Crystal Caves and collect what remains of the ancient nexus.
                </div>
              </div>
              <div>
                <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 8 }}>OBJECTIVES</div>
                {[
                  { text:"Collect Crystal Shards", cur:6, max:10, done:false },
                  { text:"Return to Elder Moros",  cur:0, max:1,  done:false },
                  { text:"Enter Crystal Caves",    cur:1, max:1,  done:true  },
                ].map((o, i) => (
                  <div key={i} style={{ display: "flex", alignItems: "flex-start", gap: 10, marginBottom: 10 }}>
                    <span style={{ marginTop: 2, fontSize: 12 }}>{o.done ? "✅" : "⬜"}</span>
                    <div style={{ flex: 1 }}>
                      <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 3 }}>
                        <span style={{ ...vtFont, fontSize: 15, color: o.done ? "#6A7FB5" : "#fff", textDecoration: o.done ? "line-through" : "none" }}>{o.text}</span>
                        {!o.done && <span style={{ ...monoFont, fontSize: 9, color: "#C7D0E0" }}>{o.cur}/{o.max}</span>}
                      </div>
                      {!o.done && <div className="sv-bar"><div className="sv-bar-fill" style={{ width: `${(o.cur/o.max)*100}%`, background: "#4DE1FF" }}/></div>}
                    </div>
                  </div>
                ))}
              </div>
              <div style={{ padding: 12, background: "rgba(255,213,74,0.08)", border: "1px solid rgba(255,213,74,0.25)", borderRadius: 8 }}>
                <div style={{ ...pxFont, fontSize: 6, color: "#FFD54A", marginBottom: 6 }}>REWARDS</div>
                {[
                  { label:"Experience",  val:"+2,500 XP",  col:"#6C5CE7" },
                  { label:"Gold",        val:"+350 Gold",  col:"#FFD700" },
                  { label:"Item",        val:"Crystal Key",col:"#4F8CFF" },
                ].map(r => (
                  <div key={r.label} style={{ display: "flex", justifyContent: "space-between", marginBottom: 4 }}>
                    <span style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>{r.label}</span>
                    <span style={{ ...vtFont, fontSize: 14, color: r.col }}>{r.val}</span>
                  </div>
                ))}
              </div>
              <div style={{ display: "flex", gap: 8 }}>
                <Btn size="sm">TRACK</Btn>
                <Btn size="sm" variant="purple">SHARE QUEST</Btn>
              </div>
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── WALLET ── */}
      {subTab === "wallet" && (
        <CrystalPanel style={{ animation: "fadeInScale .20s ease-out" }}>
          <div className="sv-panel-header sv-panel-drag">
            <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <span style={{ fontSize: 16 }}>💰</span>
              <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>WALLET</span>
              <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>Currency Overview</span>
            </div>
            <button className="sv-btn sv-btn-disabled" style={{ width: 24, height: 24, padding: 0, fontSize: 9 }}>✕</button>
          </div>

          <div style={{ padding: 16, display: "flex", gap: 16, flexWrap: "wrap" }}>

            {/* ── LEFT: Currency summaries ── */}
            <div style={{ flex: 1, minWidth: 260, display: "flex", flexDirection: "column", gap: 12 }}>

              {/* Coins card */}
              <div style={{ background: "rgba(255,215,0,0.06)", border: "1px solid rgba(255,215,0,0.28)", borderRadius: 10, padding: 16 }}>
                <div style={{ display: "flex", alignItems: "center", gap: 10, marginBottom: 12 }}>
                  <SvgIcon.Coins size={22} color="#FFD700"/>
                  <div>
                    <div style={{ ...pxFont, fontSize: 7, color: "#FFD700" }}>COINS</div>
                    <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>Standard Gameplay Currency</div>
                  </div>
                  <div style={{ marginLeft: "auto", ...monoFont, fontSize: 18, color: "#FFD700" }}>52,450</div>
                </div>
                <div style={{ display: "flex", gap: 12 }}>
                  <div style={{ flex: 1 }}>
                    <div style={{ ...pxFont, fontSize: 6, color: "#4CD964", marginBottom: 5 }}>EARN BY</div>
                    {["Farming & Mining","Breaking Blocks","Completing Quests","Selling to Players","NPC Rewards","Trading"].map(s => (
                      <div key={s} style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", display: "flex", alignItems: "center", gap: 5, marginBottom: 2 }}>
                        <span style={{ color: "#4CD964", fontSize: 10 }}>+</span>{s}
                      </div>
                    ))}
                  </div>
                  <div style={{ flex: 1 }}>
                    <div style={{ ...pxFont, fontSize: 6, color: "#FF5A5A", marginBottom: 5 }}>SPEND ON</div>
                    {["NPC Shops","World Locks","Seeds & Tools","Crafting Fees","Marketplace","Repair Costs"].map(s => (
                      <div key={s} style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", display: "flex", alignItems: "center", gap: 5, marginBottom: 2 }}>
                        <span style={{ color: "#FF5A5A", fontSize: 10 }}>−</span>{s}
                      </div>
                    ))}
                  </div>
                </div>
              </div>

              {/* Gems card */}
              <div style={{ background: "rgba(77,225,255,0.05)", border: "1px solid rgba(77,225,255,0.28)", borderRadius: 10, padding: 16 }}>
                <div style={{ display: "flex", alignItems: "center", gap: 10, marginBottom: 12 }}>
                  <SvgIcon.Gem size={22} color="#4DE1FF"/>
                  <div>
                    <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF" }}>GEMS</div>
                    <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>Premium Currency</div>
                  </div>
                  <div style={{ marginLeft: "auto", ...monoFont, fontSize: 18, color: "#4DE1FF" }}>1,250</div>
                </div>
                <div style={{ display: "flex", gap: 12 }}>
                  <div style={{ flex: 1 }}>
                    <div style={{ ...pxFont, fontSize: 6, color: "#4CD964", marginBottom: 5 }}>OBTAIN BY</div>
                    {["Purchasing Gem Packs","Promotional Events","Official Giveaways","Seasonal Rewards","Achievement Rewards (limited)"].map(s => (
                      <div key={s} style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", display: "flex", alignItems: "center", gap: 5, marginBottom: 2 }}>
                        <span style={{ color: "#4DE1FF", fontSize: 10 }}>✦</span>{s}
                      </div>
                    ))}
                  </div>
                  <div style={{ flex: 1 }}>
                    <div style={{ ...pxFont, fontSize: 6, color: "#FF5A5A", marginBottom: 5 }}>SPEND IN</div>
                    {["Premium Shop","Cosmetic Outfits","Wings & Auras","Pets & Emotes","Inventory Expansion"].map(s => (
                      <div key={s} style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", display: "flex", alignItems: "center", gap: 5, marginBottom: 2 }}>
                        <span style={{ color: "#FF5A5A", fontSize: 10 }}>−</span>{s}
                      </div>
                    ))}
                    <div style={{ marginTop: 6, padding: "5px 8px", background: "rgba(255,90,90,0.08)", border: "1px solid rgba(255,90,90,0.22)", borderRadius: 5 }}>
                      <span style={{ ...pxFont, fontSize: 5, color: "#FF5A5A" }}>NEVER farmable through gameplay</span>
                    </div>
                  </div>
                </div>
              </div>

              {/* World Lock info */}
              <div style={{ background: "rgba(106,127,181,0.08)", border: "1px solid rgba(106,127,181,0.28)", borderRadius: 10, padding: "12px 16px", display: "flex", alignItems: "center", gap: 12 }}>
                <span style={{ fontSize: 24 }}>🔒</span>
                <div style={{ flex: 1 }}>
                  <div style={{ ...pxFont, fontSize: 7, color: "#fff", marginBottom: 2 }}>WORLD LOCK</div>
                  <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>Standard gameplay item. Tradeable via Marketplace.</div>
                </div>
                <div style={{ textAlign: "right" }}>
                  <div style={{ ...monoFont, fontSize: 13, color: "#FFD700" }}>🪙 50,000</div>
                  <div style={{ ...pxFont, fontSize: 5, color: "#FF5A5A", marginTop: 2 }}>NEVER sold for Gems</div>
                </div>
              </div>
            </div>

            {/* ── RIGHT: Transaction history ── */}
            <div style={{ width: 300, flexShrink: 0 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 10 }}>TRANSACTION HISTORY</div>
              <div style={{ background: "rgba(14,18,30,0.55)", border: "1px solid rgba(106,127,181,0.22)", borderRadius: 8, overflow: "hidden" }}>
                {/* Header */}
                <div style={{ display: "grid", gridTemplateColumns: "60px 1fr 80px 50px", padding: "6px 12px", borderBottom: "1px solid rgba(106,127,181,0.20)", background: "rgba(14,18,30,0.5)" }}>
                  {["DATE","DESCRIPTION","AMOUNT","TYPE"].map(h => (
                    <span key={h} style={{ ...pxFont, fontSize: 5, color: "#6A7FB5" }}>{h}</span>
                  ))}
                </div>
                {[
                  { date:"Today",    desc:"Quest Reward",       amt:"+500",    type:"🪙", col:"#4CD964" },
                  { date:"Today",    desc:"NPC Item Purchase",  amt:"−250",    type:"🪙", col:"#FF5A5A" },
                  { date:"Today",    desc:"Gem Pack · Starter", amt:"+100",    type:"💎", col:"#4DE1FF" },
                  { date:"Today",    desc:"Crystal Wings",      amt:"−300",    type:"💎", col:"#FF5A5A" },
                  { date:"2d ago",   desc:"Marketplace Sale",   amt:"+1,200",  type:"🪙", col:"#4CD964" },
                  { date:"2d ago",   desc:"Crafting Fee",       amt:"−80",     type:"🪙", col:"#FF5A5A" },
                  { date:"3d ago",   desc:"Gem Pack · Popular", amt:"+550",    type:"💎", col:"#4DE1FF" },
                  { date:"4d ago",   desc:"Farming Session",    amt:"+340",    type:"🪙", col:"#4CD964" },
                  { date:"5d ago",   desc:"World Lock Purchase",amt:"−50,000", type:"🪙", col:"#FF5A5A" },
                  { date:"1wk ago",  desc:"Arcane Aura",        amt:"−250",    type:"💎", col:"#FF5A5A" },
                ].map((tx, i) => (
                  <div key={i} style={{ display: "grid", gridTemplateColumns: "60px 1fr 80px 50px", padding: "7px 12px", borderBottom: "1px solid rgba(106,127,181,0.12)", background: i % 2 === 0 ? "transparent" : "rgba(14,18,30,0.25)" }}>
                    <span style={{ ...monoFont, fontSize: 8, color: "#6A7FB5" }}>{tx.date}</span>
                    <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>{tx.desc}</span>
                    <span style={{ ...monoFont, fontSize: 9, color: tx.col, textAlign: "right" }}>{tx.amt}</span>
                    <span style={{ fontSize: 12, textAlign: "center" }}>{tx.type}</span>
                  </div>
                ))}
              </div>
              <div style={{ marginTop: 8, padding: "6px 10px", background: "rgba(106,127,181,0.06)", border: "1px solid rgba(106,127,181,0.18)", borderRadius: 6 }}>
                <span style={{ ...vtFont, fontSize: 12, color: "#6A7FB5" }}>Coins 🪙 and Gems 💎 are tracked in separate ledgers. Transactions are server-authoritative.</span>
              </div>
            </div>
          </div>

          {/* ── Wallet Statistics ── */}
          <div style={{ margin: "0 16px", paddingTop: 14, borderTop: "1px solid rgba(106,127,181,0.18)", marginBottom: 14 }}>
            <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 12 }}>STATISTICS</div>
            <div style={{ display: "flex", gap: 8, flexWrap: "wrap", marginBottom: 12 }}>
              {[
                { label: "Total Coins Earned", value: "184,320", color: "#FFD700" },
                { label: "Total Coins Spent",  value: "131,870", color: "#FF5A5A" },
                { label: "Total Gems Bought",  value: "1,650",   color: "#4DE1FF" },
                { label: "Total Gems Spent",   value: "300",     color: "#6C5CE7" },
                { label: "Marketplace Trades", value: "42",      color: "#4CD964" },
                { label: "Largest Tx",         value: "50,000 🪙", color: "#FFD700" },
              ].map(stat => (
                <div key={stat.label} style={{ flex: "1 1 140px", padding: "10px 12px", borderRadius: 8, border: "1px solid rgba(106,127,181,0.20)", background: "rgba(14,18,30,0.50)", display: "flex", flexDirection: "column", gap: 4 }}>
                  <span style={{ ...pxFont, fontSize: 5, color: "#6A7FB5" }}>{stat.label}</span>
                  <span style={{ ...monoFont, fontSize: 16, color: stat.color }}>{stat.value}</span>
                </div>
              ))}
            </div>
            <div style={{ marginBottom: 4 }}>
              <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 4 }}>
                <span style={{ ...vtFont, fontSize: 13, color: "#FFD700" }}>🪙 Coins 97.6%</span>
                <span style={{ ...vtFont, fontSize: 13, color: "#4DE1FF" }}>💎 Gems 2.4%</span>
              </div>
              <div style={{ height: 8, borderRadius: 4, overflow: "hidden", display: "flex" }}>
                <div style={{ width: "97.6%", background: "#FFD700", opacity: 0.8 }}/>
                <div style={{ flex: 1, background: "#4DE1FF", opacity: 0.8 }}/>
              </div>
              <div style={{ ...vtFont, fontSize: 12, color: "#6A7FB5", marginTop: 4 }}>Balance ratio — total portfolio split</div>
            </div>
          </div>

          {/* ── Gem Pack purchase ── */}
          <div style={{ margin: "0 16px 16px", padding: "14px 16px", background: "rgba(14,18,30,0.55)", border: "1px solid rgba(106,127,181,0.22)", borderRadius: 10 }}>
            <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 12 }}>GET GEMS — OFFICIAL PACKS</div>
            <div style={{ display: "flex", gap: 10, flexWrap: "wrap" }}>
              {[
                { label:"STARTER",  gems:100,  price:"$0.99",  badge:null,          color:"#C7D0E0" },
                { label:"POPULAR",  gems:550,  price:"$4.99",  badge:"BEST VALUE",  color:"#FFD700" },
                { label:"VALUE",    gems:1200, price:"$9.99",  badge:"SAVE 20%",    color:"#4F8CFF" },
                { label:"PREMIUM",  gems:2800, price:"$19.99", badge:"SAVE 30%",    color:"#6C5CE7" },
              ].map(pack => (
                <div key={pack.label} style={{ flex: 1, minWidth: 120, position: "relative", background: "rgba(14,18,30,0.70)", border: `1px solid ${pack.color}44`, borderRadius: 9, padding: "14px 12px", display: "flex", flexDirection: "column", alignItems: "center", gap: 6 }}>
                  {pack.badge && (
                    <div style={{ position: "absolute", top: -8, left: "50%", transform: "translateX(-50%)", ...pxFont, fontSize: 5, color: "#1E2230", background: pack.color, padding: "2px 8px", borderRadius: 4, whiteSpace: "nowrap" }}>
                      {pack.badge}
                    </div>
                  )}
                  <SvgIcon.Gem size={28} color={pack.color}/>
                  <div style={{ ...pxFont, fontSize: 7, color: pack.color }}>{pack.label}</div>
                  <div style={{ ...monoFont, fontSize: 14, color: "#fff" }}>💎 {pack.gems.toLocaleString()}</div>
                  <div style={{ ...monoFont, fontSize: 10, color: "#C7D0E0" }}>{pack.price}</div>
                  <button className="sv-btn sv-btn-disabled" style={{ fontSize: 7, padding: "5px 14px", width: "100%", marginTop: 4 }} title="Sign in to purchase">BUY</button>
                </div>
              ))}
            </div>
            <div style={{ marginTop: 12, ...vtFont, fontSize: 12, color: "#6A7FB5", textAlign: "center" }}>
              Gems are a premium currency. All purchases are final and non-refundable. Gems are non-transferable and have no real-world value.
            </div>
          </div>
        </CrystalPanel>
      )}

      {/* ── UNIFIED SHOP ── */}
      {subTab === "shop" && (() => {
        const isCoin    = shopSection === "coin";
        const ac        = isCoin ? "#FFD700" : "#4DE1FF";
        const acDim     = isCoin ? "rgba(255,215,0,0.22)" : "rgba(77,225,255,0.22)";
        const cats      = isCoin ? COIN_CATS : GEM_CATS;
        const activeCat = isCoin ? coinCat : gemCat;
        const setActiveCat = isCoin
          ? (c: string) => setCoinCat(c as CoinCat)
          : (c: string) => setGemCat(c as GemCat);
        const rawItems  = isCoin
          ? (COIN_ITEMS[coinCat] ?? [])
          : (GEM_ITEMS[gemCat] ?? []);
        const items = shopSearch.trim()
          ? rawItems.filter(it =>
              it.name.toLowerCase().includes(shopSearch.toLowerCase()) ||
              it.desc.toLowerCase().includes(shopSearch.toLowerCase()))
          : rawItems;
        const badgeColor = (b: string) =>
          b === "NEW" || b === "POPULAR" ? "#4CD964" :
          b === "LIMITED" || b === "RARE" ? "#FF5A5A" :
          b === "SEASONAL" ? "#6C5CE7" :
          b === "BEST VALUE" || b === "SAVE 20%" || b === "SAVE 30%" ? "#FFD700" :
          b === "ESSENTIAL" || b === "BEST" || b === "ADVANCED" ? "#4F8CFF" : "#C7D0E0";

        return (
          <CrystalPanel style={{ animation: "fadeInScale .20s ease-out", position: "relative" }}>

            {/* ── Confirm dialog overlay ── */}
            {confirmItem && (
              <div style={{ position: "absolute", inset: 0, background: "rgba(10,14,24,0.80)", backdropFilter: "blur(4px)", display: "flex", alignItems: "center", justifyContent: "center", zIndex: 50, borderRadius: 10 }}>
                <div style={{ width: 340, background: "#1E2230", border: `1px solid ${confirmItem.currency === "coin" ? "rgba(255,215,0,0.40)" : "rgba(77,225,255,0.40)"}`, borderRadius: 12, padding: 24, boxShadow: "0 8px 40px rgba(0,0,0,0.7)", animation: "fadeInScale .2s ease-out" }}>
                  <div style={{ ...pxFont, fontSize: 9, color: "#fff", marginBottom: 4 }}>CONFIRM PURCHASE</div>
                  <div style={{ ...vtFont, fontSize: 13, color: "#6A7FB5", marginBottom: 18 }}>Server will process this transaction</div>
                  <div style={{ display: "flex", alignItems: "center", gap: 14, padding: "14px 16px", background: "rgba(14,18,30,0.60)", border: "1px solid rgba(106,127,181,0.20)", borderRadius: 9, marginBottom: 16 }}>
                    <span style={{ fontSize: 40, flexShrink: 0 }}>{confirmItem.emoji}</span>
                    <div>
                      <div style={{ ...pxFont, fontSize: 8, color: "#fff" }}>{confirmItem.name}</div>
                      <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", marginTop: 2 }}>{confirmItem.desc}</div>
                    </div>
                  </div>
                  <div style={{ display: "flex", flexDirection: "column", gap: 6, marginBottom: 18 }}>
                    {[
                      { label:"Currency",  val: confirmItem.currency === "coin" ? "🪙 Coins" : "💎 Gems" },
                      { label:"Price",     val: `${confirmItem.currency === "coin" ? "🪙" : "💎"} ${confirmItem.price.toLocaleString()}` },
                      { label:"Balance",   val: confirmItem.currency === "coin" ? "🪙 52,450" : "💎 1,250" },
                      { label:"Remaining", val: confirmItem.currency === "coin"
                          ? `🪙 ${(52450 - confirmItem.price).toLocaleString()}`
                          : `💎 ${(1250  - confirmItem.price).toLocaleString()}` },
                    ].map(r => (
                      <div key={r.label} style={{ display: "flex", justifyContent: "space-between" }}>
                        <span style={{ ...vtFont, fontSize: 14, color: "#6A7FB5" }}>{r.label}</span>
                        <span style={{ ...monoFont, fontSize: 10, color: "#fff" }}>{r.val}</span>
                      </div>
                    ))}
                  </div>
                  <div style={{ padding: "8px 12px", background: "rgba(79,140,255,0.07)", border: "1px solid rgba(79,140,255,0.20)", borderRadius: 6, marginBottom: 16 }}>
                    <span style={{ ...vtFont, fontSize: 13, color: "#6A7FB5" }}>
                      Purchase request will be sent to the server. Balance will only update after server confirmation.
                    </span>
                  </div>
                  <div style={{ display: "flex", gap: 10 }}>
                    <button className="sv-btn" style={{ flex: 1, borderColor: `${ac}66`, boxShadow: `0 0 10px ${ac}22` }}
                      onClick={() => setConfirmItem(null)}>
                      ✓ CONFIRM
                    </button>
                    <button className="sv-btn" style={{ flex: 1, background: "rgba(44,49,69,0.70)", borderColor: "rgba(106,127,181,0.30)", boxShadow: "none" }}
                      onClick={() => setConfirmItem(null)}>
                      CANCEL
                    </button>
                  </div>
                </div>
              </div>
            )}

            {/* ── TOP BAR ── */}
            <div className="sv-panel-header sv-panel-drag" style={{ gap: 12, flexWrap: "wrap" }}>
              {/* Title */}
              <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
                <span style={{ fontSize: 16 }}>🏪</span>
                <span style={{ ...pxFont, fontSize: 10, color: "#fff" }}>SERVER SHOP</span>
                <span style={{ ...pxFont, fontSize: 5, color: "#4CD964", background: "rgba(76,217,100,0.12)", border: "1px solid rgba(76,217,100,0.25)", padding: "2px 7px", borderRadius: 4 }}>NO PAY-TO-WIN</span>
              </div>
              {/* Search */}
              <input
                className="sv-input"
                placeholder="Search items..."
                value={shopSearch}
                onChange={e => setShopSearch(e.target.value)}
                style={{ width: 180, ...vtFont, fontSize: 14, padding: "4px 10px" }}
              />
              {/* Refresh */}
              <button className="sv-btn" style={{ padding: "5px 10px", fontSize: 8 }}>↺</button>
              <div style={{ flex: 1 }}/>
              {/* Balances */}
              <div style={{ display: "flex", alignItems: "center", gap: 5, background: "rgba(255,215,0,0.08)", border: "1px solid rgba(255,215,0,0.28)", borderRadius: 6, padding: "4px 10px" }}>
                <SvgIcon.Coins size={12} color="#FFD700"/>
                <span style={{ ...monoFont, fontSize: 11, color: "#FFD700" }}>52,450</span>
              </div>
              <div style={{ display: "flex", alignItems: "center", gap: 5, background: "rgba(77,225,255,0.08)", border: "1px solid rgba(77,225,255,0.28)", borderRadius: 6, padding: "4px 10px" }}>
                <SvgIcon.Gem size={12} color="#4DE1FF"/>
                <span style={{ ...monoFont, fontSize: 11, color: "#4DE1FF" }}>1,250</span>
              </div>
              <button className="sv-btn sv-btn-disabled" style={{ width: 24, height: 24, padding: 0, fontSize: 9 }}>✕</button>
            </div>

            {/* ── SECTION TABS ── */}
            <div style={{ display: "flex", gap: 0, borderBottom: "1px solid rgba(106,127,181,0.22)", padding: "0 16px" }}>
              {([["coin","🪙 COIN SHOP","#FFD700"],["gem","💎 PREMIUM SHOP","#4DE1FF"]] as [ShopSection,string,string][]).map(([sec,label,color]) => (
                <button key={sec} onClick={() => { setShopSection(sec); setShopSearch(""); }}
                  style={{ ...pxFont, fontSize: 7, padding: "10px 20px", cursor: "pointer", color: shopSection === sec ? color : "#C7D0E0", borderBottom: shopSection === sec ? `2px solid ${color}` : "2px solid transparent", background: "transparent", marginBottom: -1, transition: "all .14s",
                    textShadow: shopSection === sec ? `0 0 10px ${color}55` : "none" }}>
                  {label}
                </button>
              ))}
              <div style={{ flex: 1 }}/>
              <span style={{ ...vtFont, fontSize: 13, color: "#6A7FB5", alignSelf: "center" }}>
                {isCoin ? "Earned through gameplay · Never Gems" : "Cosmetics only · No gameplay advantages"}
              </span>
            </div>

            {/* ── BODY: sidebar + grid ── */}
            <div style={{ display: "flex", minHeight: 0 }}>

              {/* Category sidebar */}
              <div style={{ width: 150, flexShrink: 0, borderRight: "1px solid rgba(106,127,181,0.18)", padding: "12px 0", overflowY: "auto" }} className="sv-scroll">
                {cats.map(cat => {
                  const isActive = cat === activeCat;
                  return (
                    <button key={cat} onClick={() => { setActiveCat(cat); setShopSearch(""); }}
                      style={{ display: "block", width: "100%", textAlign: "left", padding: "8px 14px", ...vtFont, fontSize: 14, color: isActive ? ac : "#C7D0E0", background: isActive ? `${ac}12` : "transparent", borderLeft: `2px solid ${isActive ? ac : "transparent"}`, cursor: "pointer", transition: "all .12s" }}
                      onMouseEnter={e => { if (!isActive) { (e.currentTarget as HTMLElement).style.background = "rgba(106,127,181,0.08)"; (e.currentTarget as HTMLElement).style.color = "#fff"; }}}
                      onMouseLeave={e => { if (!isActive) { (e.currentTarget as HTMLElement).style.background = "transparent"; (e.currentTarget as HTMLElement).style.color = "#C7D0E0"; }}}>
                      {cat}
                    </button>
                  );
                })}
              </div>

              {/* Item grid area */}
              <div className="sv-scroll" style={{ flex: 1, padding: 14, overflowY: "auto", maxHeight: 500 }}>
                {/* Category header */}
                <div style={{ display: "flex", alignItems: "center", gap: 10, marginBottom: 14 }}>
                  <div style={{ width: 3, height: 16, background: ac, borderRadius: 2, flexShrink: 0 }}/>
                  <span style={{ ...pxFont, fontSize: 8, color: ac }}>{activeCat.toUpperCase()}</span>
                  <span style={{ ...vtFont, fontSize: 13, color: "#6A7FB5" }}>· {items.length} items</span>
                  {isCoin
                    ? <span style={{ ...vtFont, fontSize: 13, color: "rgba(255,215,0,0.55)", marginLeft: "auto" }}>🪙 Coins only</span>
                    : <span style={{ ...vtFont, fontSize: 13, color: "rgba(77,225,255,0.55)", marginLeft: "auto" }}>💎 Gems only · Cosmetic</span>}
                </div>

                {items.length === 0 && (
                  <div style={{ textAlign: "center", padding: 40 }}>
                    <div style={{ fontSize: 32, marginBottom: 8 }}>🔍</div>
                    <div style={{ ...vtFont, fontSize: 18, color: "#6A7FB5" }}>No items match your search.</div>
                  </div>
                )}

                {/* Featured banner for category "Featured" / "Featured Premium" */}
                {(activeCat === "Featured" || activeCat === "Featured Premium") && !shopSearch && (
                  <div style={{ background: `linear-gradient(135deg, ${ac}14 0%, rgba(108,92,231,0.14) 100%)`, border: `1px solid ${ac}44`, borderRadius: 12, padding: 16, display: "flex", gap: 16, alignItems: "center", marginBottom: 14, position: "relative", overflow: "hidden" }}>
                    <div style={{ position: "absolute", top: -18, right: -18, width: 100, height: 100, borderRadius: "50%", background: `${ac}08`, pointerEvents: "none" }}/>
                    <div style={{ fontSize: 44, flexShrink: 0 }}>{items[0]?.emoji ?? "⭐"}</div>
                    <div style={{ flex: 1 }}>
                      <div style={{ display: "flex", gap: 6, marginBottom: 6 }}>
                        <span style={{ ...pxFont, fontSize: 5, color: "#FFD700", background: "rgba(255,215,0,0.14)", padding: "2px 7px", borderRadius: 4 }}>FEATURED</span>
                        {items[0]?.badge && <span style={{ ...pxFont, fontSize: 5, color: badgeColor(items[0].badge), background: `${badgeColor(items[0].badge)}18`, padding: "2px 7px", borderRadius: 4 }}>{items[0].badge}</span>}
                      </div>
                      <div style={{ ...pxFont, fontSize: 9, color: "#fff", marginBottom: 4 }}>{items[0]?.name}</div>
                      <div style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>{items[0]?.desc}</div>
                    </div>
                    <div style={{ textAlign: "center", flexShrink: 0 }}>
                      <div style={{ ...monoFont, fontSize: 18, color: ac, marginBottom: 8 }}>
                        {isCoin ? "🪙" : "💎"} {items[0]?.price.toLocaleString()}
                      </div>
                      <button className="sv-btn" style={{ fontSize: 7, padding: "8px 20px", borderColor: `${ac}66`, boxShadow: `0 0 12px ${ac}28` }}
                        onClick={() => items[0] && setConfirmItem({ ...items[0], currency: isCoin ? "coin" : "gem" })}>
                        BUY
                      </button>
                    </div>
                  </div>
                )}

                {/* Item grid */}
                <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(155px, 1fr))", gap: 10 }}>
                  {items.map((item, i) => (
                    <div key={i} style={{ background: "rgba(14,18,30,0.60)", border: "1px solid rgba(106,127,181,0.22)", borderRadius: 10, padding: "12px 10px", display: "flex", flexDirection: "column", alignItems: "center", gap: 6, position: "relative", cursor: "pointer", transition: "border-color .12s, box-shadow .12s" }}
                      onMouseEnter={e => { (e.currentTarget as HTMLElement).style.borderColor = `${ac}55`; (e.currentTarget as HTMLElement).style.boxShadow = `0 0 12px ${ac}18`; setTooltip({ name: item.name, description: item.desc, rarity: "common", stats: [{ label: "Price", value: `${isCoin ? "🪙" : "💎"} ${item.price.toLocaleString()}`, color: ac }] }); setTooltipPos({ x: e.clientX, y: e.clientY }); }}
                      onMouseMove={e => setTooltipPos({ x: e.clientX, y: e.clientY })}
                      onMouseLeave={e => { (e.currentTarget as HTMLElement).style.borderColor = "rgba(106,127,181,0.22)"; (e.currentTarget as HTMLElement).style.boxShadow = "none"; setTooltip(null); }}>
                      {item.badge && (
                        <div style={{ position: "absolute", top: 7, right: 7, ...pxFont, fontSize: 5, color: "#1E2230", background: badgeColor(item.badge), padding: "1px 5px", borderRadius: 3, whiteSpace: "nowrap" }}>
                          {item.badge}
                        </div>
                      )}
                      <div style={{ fontSize: 34, lineHeight: 1, marginTop: item.badge ? 8 : 0 }}>{item.emoji}</div>
                      <span style={{ ...vtFont, fontSize: 14, color: "#fff", textAlign: "center", lineHeight: 1.3 }}>{item.name}</span>
                      <span style={{ ...vtFont, fontSize: 12, color: "#6A7FB5", textAlign: "center", lineHeight: 1.3 }}>{item.desc}</span>
                      {/* Currency + price */}
                      <div style={{ display: "flex", alignItems: "center", gap: 5, padding: "3px 8px", background: `${ac}10`, border: `1px solid ${acDim}`, borderRadius: 5 }}>
                        {isCoin
                          ? <SvgIcon.Coins size={11} color="#FFD700"/>
                          : <SvgIcon.Gem size={11} color="#4DE1FF"/>}
                        <span style={{ ...monoFont, fontSize: 11, color: ac }}>{item.price.toLocaleString()}</span>
                      </div>
                      <button className="sv-btn" style={{ fontSize: 6, padding: "5px 12px", width: "100%", borderColor: `${ac}44` }}
                        onClick={() => setConfirmItem({ ...item, currency: isCoin ? "coin" : "gem" })}>
                        BUY
                      </button>
                    </div>
                  ))}
                </div>

                {/* Bottom notice */}
                <div style={{ marginTop: 14, display: "flex", alignItems: "flex-start", gap: 10, padding: "10px 14px", background: isCoin ? "rgba(255,215,0,0.05)" : "rgba(76,217,100,0.05)", border: `1px solid ${isCoin ? "rgba(255,215,0,0.20)" : "rgba(76,217,100,0.20)"}`, borderRadius: 8 }}>
                  <span style={{ fontSize: 15, flexShrink: 0 }}>{isCoin ? "🪙" : "🛡️"}</span>
                  <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", lineHeight: 1.5 }}>
                    {isCoin
                      ? "Prices are server-authoritative. Your Coin balance is never deducted locally before server confirmation."
                      : "The Premium Shop never sells weapons, armor, stat boosts, or gameplay advantages. All items are purely cosmetic. Gems cannot be earned through gameplay."}
                  </span>
                </div>
              </div>
            </div>
          </CrystalPanel>
        );
      })()}

      {tooltip && <UITooltip item={tooltip} x={tooltipPos.x} y={tooltipPos.y}/>}
    </div>
  );
}

// ─── ICONS TAB ────────────────────────────────────────────────────────────────
function IconsTab() {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 20 }}>
      <CrystalPanel>
        <PanelHeader title="ICON PACK — CRYSTAL PIXEL STYLE"/>
        <div style={{ padding: 20, display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(90px,1fr))", gap: 10 }}>
          {ICON_LIST.map(([name, Icon]) => (
            <div key={name} style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 8, padding: "12px 8px", borderRadius: 7, border: "1px solid rgba(106,127,181,0.22)", cursor: "pointer", transition: "all .12s" }}
              onMouseEnter={e => { (e.currentTarget as HTMLElement).style.background = "rgba(77,225,255,0.05)"; (e.currentTarget as HTMLElement).style.borderColor = "rgba(77,225,255,0.35)"; }}
              onMouseLeave={e => { (e.currentTarget as HTMLElement).style.background = "transparent"; (e.currentTarget as HTMLElement).style.borderColor = "rgba(106,127,181,0.22)"; }}>
              <Icon size={24}/>
              <span style={{ ...pxFont, fontSize: 6, color: "#C7D0E0", textAlign: "center" }}>{name.toUpperCase()}</span>
            </div>
          ))}
        </div>
      </CrystalPanel>

      <CrystalPanel>
        <PanelHeader title="ICON SIZES + COLORS"/>
        <div style={{ padding: 20, display: "flex", flexDirection: "column", gap: 20 }}>
          <div>
            <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 12 }}>SIZE SCALE</div>
            <div style={{ display: "flex", alignItems: "flex-end", gap: 24 }}>
              {[12, 16, 20, 24, 32, 48].map(sz => (
                <div key={sz} style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 8 }}>
                  <SvgIcon.Health size={sz} color="#FF5A5A"/>
                  <span style={{ ...monoFont, fontSize: 9, color: "#C7D0E0" }}>{sz}px</span>
                </div>
              ))}
            </div>
          </div>
          <div>
            <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 12 }}>COLOR STATES</div>
            <div style={{ display: "flex", gap: 20, flexWrap: "wrap" }}>
              {[
                { label: "Default",  color: "#4DE1FF" },
                { label: "Active",   color: "#4F8CFF" },
                { label: "Warning",  color: "#FFD54A" },
                { label: "Danger",   color: "#FF5A5A" },
                { label: "Success",  color: "#4CD964" },
                { label: "Gold",     color: "#FFD700" },
                { label: "Muted",    color: "#6A7FB5" },
              ].map(c => (
                <div key={c.label} style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 6 }}>
                  <SvgIcon.Settings size={20} color={c.color}/>
                  <span style={{ ...vtFont, fontSize: 12, color: "#C7D0E0" }}>{c.label}</span>
                </div>
              ))}
            </div>
          </div>
        </div>
      </CrystalPanel>
    </div>
  );
}

// ─── ITEM SLOTS TAB ───────────────────────────────────────────────────────────
function SlotsTab() {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 20 }}>
      <CrystalPanel>
        <PanelHeader title="SLOT STATES"/>
        <div style={{ padding: 24, display: "flex", flexWrap: "wrap", gap: 28 }}>
          {[
            { label:"EMPTY",     cls:"",                  content: null,                           note: "No item" },
            { label:"HOVER",     cls:"",                  content: null,   hover: true,            note: "Mouse over" },
            { label:"SELECTED",  cls:"sv-slot-selected",  content:"💎",                            note: "Active slot" },
            { label:"LOCKED",    cls:"sv-slot-locked",    content:"🔒",                            note: "Unavailable" },
            { label:"COMMON",    cls:"",                  content:"🧪",                            note: "Common rarity" },
            { label:"RARE",      cls:"sv-slot-rare",      content:"🗝️",                           note: "#4F8CFF glow" },
            { label:"EPIC",      cls:"sv-slot-epic",      content:"🪄",                            note: "#9B59B6 glow" },
            { label:"LEGENDARY", cls:"sv-slot-legendary", content:"🔮",                            note: "#FFD700 shimmer" },
          ].map(s => (
            <div key={s.label} style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 8 }}>
              <div className={`sv-slot ${s.cls}`} style={s.hover ? { borderColor: "rgba(77,225,255,.62)", boxShadow: "0 0 10px rgba(77,225,255,.28)" } : undefined}>
                {s.content && <span style={{ fontSize: 22 }}>{s.content}</span>}
              </div>
              <span style={{ ...pxFont, fontSize: 6, color: "#C7D0E0" }}>{s.label}</span>
              <span style={{ ...vtFont, fontSize: 12, color: "#6A7FB5" }}>{s.note}</span>
            </div>
          ))}
        </div>
      </CrystalPanel>

      <CrystalPanel>
        <PanelHeader title="STACK COUNTS + DURABILITY BARS"/>
        <div style={{ padding: 24, display: "flex", gap: 20, flexWrap: "wrap" }}>
          {[
            { emoji:"💊", qty:99,  dur:100, label:"Full stack + full dur"  },
            { emoji:"🪙", qty:999, dur:null, label:"Max coins stack"       },
            { emoji:"⚔️", qty:null,dur:30,  label:"Damaged weapon"        },
            { emoji:"🛡️", qty:null,dur:75,  label:"Medium durability"     },
            { emoji:"🌿", qty:12,  dur:null, label:"Small stack"           },
          ].map((s, i) => (
            <div key={i} style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 6 }}>
              <div className="sv-slot" style={{ width: 60, height: 60 }}>
                <span style={{ fontSize: 24 }}>{s.emoji}</span>
                {s.qty != null && <span style={{ ...pxFont, position: "absolute", bottom: 1, right: 2, fontSize: 6, color: "#FFD700" }}>{s.qty > 99 ? "99+" : s.qty}</span>}
                {s.dur != null && (
                  <div style={{ position: "absolute", bottom: 3, left: 4, right: 4, height: 3, borderRadius: 2, background: "rgba(0,0,0,0.5)" }}>
                    <div style={{ height: "100%", width: `${s.dur}%`, borderRadius: 2, background: s.dur > 60 ? "#4CD964" : s.dur > 30 ? "#FFD54A" : "#FF5A5A", transition: "width .4s" }}/>
                  </div>
                )}
              </div>
              <span style={{ ...vtFont, fontSize: 12, color: "#C7D0E0", textAlign: "center", maxWidth: 80 }}>{s.label}</span>
            </div>
          ))}
        </div>
      </CrystalPanel>

      <CrystalPanel>
        <PanelHeader title="RARITY LABELS"/>
        <div style={{ padding: 20, display: "flex", gap: 16, flexWrap: "wrap", alignItems: "center" }}>
          {[
            { label:"COMMON",    color:"#C7D0E0" },
            { label:"UNCOMMON",  color:"#4CD964" },
            { label:"RARE",      color:"#4F8CFF" },
            { label:"EPIC",      color:"#9B59B6" },
            { label:"LEGENDARY", color:"#FFD700" },
            { label:"MYTHIC",    color:"#FF5A5A" },
          ].map(r => (
            <div key={r.label} style={{ padding: "5px 14px", borderRadius: 5, border: `1px solid ${r.color}55`, background: `${r.color}11` }}>
              <span style={{ ...pxFont, fontSize: 7, color: r.color, textShadow: `0 0 8px ${r.color}66` }}>{r.label}</span>
            </div>
          ))}
        </div>
      </CrystalPanel>
    </div>
  );
}

// ─── NOTIFICATIONS TAB ────────────────────────────────────────────────────────
type Notif = { id: number; icon: string; title: string; msg: string; color: string; border: string };

const NOTIF_PRESETS: Omit<Notif, "id">[] = [
  { icon:"🏆", title:"ACHIEVEMENT UNLOCKED", msg:"Crystal Collector: Gather 100 shards",   color:"#FFD700", border:"#FFD700" },
  { icon:"⭐", title:"QUEST COMPLETE",       msg:"The Crystal Hunt — +500 XP, +50 Gold",   color:"#FFD54A", border:"#FFD54A" },
  { icon:"⚔️", title:"ITEM OBTAINED",        msg:"Legendary Crystal Blade acquired",        color:"#9B59B6", border:"#9B59B6" },
  { icon:"✨", title:"LEVEL UP!",            msg:"You are now Level 43!",                    color:"#4DE1FF", border:"#4DE1FF" },
  { icon:"👤", title:"PLAYER JOINED",        msg:"CrystalKnight entered your world",        color:"#4CD964", border:"#4CD964" },
  { icon:"💨", title:"PLAYER LEFT",          msg:"ShadowRogue left the world",              color:"#C7D0E0", border:"#6A7FB5" },
  { icon:"⚠️", title:"WARNING",              msg:"Health critically low! Use a potion",    color:"#FFD54A", border:"#FFD54A" },
  { icon:"❌", title:"ERROR",                msg:"Cannot place block in protected area",    color:"#FF5A5A", border:"#FF5A5A" },
  { icon:"📡", title:"CONNECTION LOST",      msg:"Server timeout — reconnecting...",        color:"#FF5A5A", border:"#FF5A5A" },
];

function NotificationsTab() {
  const [live, setLive] = useState<Notif[]>([]);
  const [cnt, setCnt] = useState(0);

  const addNotif = (p: Omit<Notif, "id">) => {
    const id = cnt + 1;
    setCnt(id);
    setLive(prev => [{ ...p, id }, ...prev].slice(0, 5));
  };

  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 20 }}>
      <CrystalPanel>
        <PanelHeader title="TRIGGER NOTIFICATIONS"/>
        <div style={{ padding: 20 }}>
          <div style={{ display: "flex", flexWrap: "wrap", gap: 8 }}>
            {NOTIF_PRESETS.map(p => (
              <button key={p.title} className="sv-btn" style={{ fontSize: 7, padding: "6px 12px", borderColor: `${p.border}55` }} onClick={() => addNotif(p)}>
                {p.icon} {p.title.split(" ")[0]}
              </button>
            ))}
          </div>
        </div>
      </CrystalPanel>

      <CrystalPanel>
        <PanelHeader title="LIVE STACK"/>
        <div style={{ padding: 20, minHeight: 160, display: "flex", flexDirection: "column", gap: 10 }}>
          {live.length === 0 && (
            <div style={{ display: "flex", alignItems: "center", justifyContent: "center", height: 120 }}>
              <span style={{ ...vtFont, fontSize: 16, color: "#6A7FB5" }}>Click a trigger above to fire a notification</span>
            </div>
          )}
          {live.map(n => (
            <div key={n.id} className="sv-notif" style={{ borderColor: n.border }}>
              <span style={{ fontSize: 20, flexShrink: 0 }}>{n.icon}</span>
              <div style={{ flex: 1 }}>
                <div style={{ ...pxFont, fontSize: 7, color: n.color, marginBottom: 3 }}>{n.title}</div>
                <div style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>{n.msg}</div>
              </div>
              <button onClick={() => setLive(p => p.filter(x => x.id !== n.id))}
                style={{ background: "none", border: "none", cursor: "pointer", ...pxFont, fontSize: 8, color: "#6A7FB5", flexShrink: 0, padding: 4 }}
                onMouseEnter={e => { (e.currentTarget as HTMLElement).style.color = "#fff"; }}
                onMouseLeave={e => { (e.currentTarget as HTMLElement).style.color = "#6A7FB5"; }}>
                ✕
              </button>
            </div>
          ))}
        </div>
      </CrystalPanel>

      <CrystalPanel>
        <PanelHeader title="ALL STYLES — STATIC REFERENCE"/>
        <div style={{ padding: 20, display: "flex", flexDirection: "column", gap: 8 }}>
          {NOTIF_PRESETS.map(n => (
            <div key={n.title} className="sv-notif" style={{ borderColor: n.border }}>
              <span style={{ fontSize: 18, flexShrink: 0 }}>{n.icon}</span>
              <div style={{ flex: 1 }}>
                <div style={{ ...pxFont, fontSize: 7, color: n.color, marginBottom: 2 }}>{n.title}</div>
                <div style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>{n.msg}</div>
              </div>
            </div>
          ))}
        </div>
      </CrystalPanel>
    </div>
  );
}

// ─── SCREENS TAB ─────────────────────────────────────────────────────────────
const LOADING_TIPS = [
  "World Locks protect your build. Always lock your world before going offline!",
  "Plant Crystal Seeds near water tiles for faster growth.",
  "Check the Marketplace daily — rare items often appear at discount.",
  "Completing quests grants bonus XP, Coins, and crafting recipes.",
  "Gems can only be obtained via official packs or seasonal events.",
  "Join a Guild to unlock co-op dungeon access and guild rewards.",
];

type Screen = "splash" | "login" | "register" | "connecting" | "continue" | "worldselect" | "loading" | "pause" | "settings" | "credits" | "error";

function ScreensTab() {
  const [screen, setScreen] = useState<Screen>("splash");
  const [wsQuery, setWsQuery] = useState("");
  const [wsFilter, setWsFilter] = useState<"all"|"recent"|"favorites">("all");
  const [tipIdx] = useState(() => Math.floor(Math.random() * LOADING_TIPS.length));
  const SCREENS: { id: Screen; label: string }[] = [
    { id: "splash",      label: "SPLASH"      },
    { id: "login",       label: "LOGIN"       },
    { id: "register",    label: "REGISTER"    },
    { id: "connecting",  label: "CONNECTING"  },
    { id: "continue",    label: "CONTINUE"    },
    { id: "worldselect", label: "WORLD SELECT"},
    { id: "loading",     label: "LOADING"     },
    { id: "pause",       label: "PAUSE"       },
    { id: "settings",    label: "SETTINGS"    },
    { id: "credits",     label: "CREDITS"     },
    { id: "error",       label: "ERROR"       },
  ];

  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
      <div style={{ display: "flex", gap: 4 }}>
        {SCREENS.map(s => (
          <button key={s.id} onClick={() => setScreen(s.id)}
            className={`sv-tab ${screen === s.id ? "sv-tab-active" : ""}`}
            style={{ background: screen === s.id ? "rgba(79,140,255,0.10)" : "rgba(44,49,69,0.55)", borderRadius: "8px 8px 0 0", borderTop: "1px solid rgba(106,127,181,0.28)", borderLeft: "1px solid rgba(106,127,181,0.28)", borderRight: "1px solid rgba(106,127,181,0.28)", borderBottom: "none" }}>
            {s.label}
          </button>
        ))}
      </div>

      {/* ── SPLASH ── */}
      {screen === "splash" && (
        <div className="sv-pixel-grid" style={{ height: 460, borderRadius: 10, background: "linear-gradient(160deg,#06090f 0%,#0d1525 50%,#080d18 100%)", display: "flex", alignItems: "center", justifyContent: "center", position: "relative", overflow: "hidden" }}>
          <div style={{ position: "absolute", inset: 0, backgroundImage: "radial-gradient(ellipse at 50% 35%, rgba(79,140,255,0.12) 0%, transparent 60%)" }}/>
          {/* Decorative crystal shards */}
          {[[10,15,20],[85,10,16],[5,70,14],[90,75,18],[50,85,12]].map(([l,t,sz],i) => (
            <div key={i} style={{ position:"absolute", left:`${l}%`, top:`${t}%`, width: sz, height: sz * 2, background: "rgba(77,225,255,0.08)", transform: "rotate(30deg)", borderRadius: 2, border:"1px solid rgba(77,225,255,0.15)" }}/>
          ))}
          <div style={{ textAlign: "center", animation: "fadeInScale .9s ease-out", zIndex: 1 }}>
            <div style={{ ...vtFont, fontSize: 14, color: "#6C5CE7", letterSpacing: "0.3em", marginBottom: 8 }}>WELCOME TO</div>
            <div style={{ ...pxFont, fontSize: 44, color: "#fff", textShadow: "0 0 50px rgba(77,225,255,0.95), 0 0 100px rgba(79,140,255,0.55), 4px 4px 0 #000", marginBottom: 12, letterSpacing: "0.06em" }}>
              STRIXVERSE
            </div>
            <div style={{ ...pxFont, fontSize: 10, color: "#4DE1FF", textShadow: "0 0 14px rgba(77,225,255,0.75)", marginBottom: 48, letterSpacing: "0.16em" }}>
              CRYSTAL TECHNOLOGY · FANTASY WORLD
            </div>
            <div style={{ animation: "glowPulse 1.6s ease-in-out infinite" }}>
              <span style={{ ...vtFont, fontSize: 22, color: "#C7D0E0" }}>PRESS ANY KEY TO CONTINUE</span>
            </div>
          </div>
          <div style={{ position: "absolute", bottom: 16, left: 0, right: 0, display: "flex", justifyContent: "space-between", padding: "0 24px" }}>
            <span style={{ ...monoFont, fontSize: 9, color: "rgba(199,208,224,0.30)" }}>v2.1.4 — Crystal Build</span>
            <span style={{ ...monoFont, fontSize: 9, color: "rgba(199,208,224,0.30)" }}>StrixVerse Studios © 2025</span>
          </div>
        </div>
      )}

      {/* ── LOGIN ── */}
      {screen === "login" && (
        <div style={{ height: 460, borderRadius: 10, background: "linear-gradient(135deg,#090e1a 0%,#101830 100%)", display: "flex", overflow: "hidden" }}>
          {/* Left brand + player flow */}
          <div className="sv-pixel-grid" style={{ flex: 1, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 18, padding: 32 }}>
            <div style={{ ...pxFont, fontSize: 26, color: "#fff", textShadow: "0 0 28px rgba(77,225,255,0.80), 3px 3px 0 #000" }}>STRIXVERSE</div>
            <div style={{ ...pxFont, fontSize: 8, color: "#4DE1FF", letterSpacing: "0.14em" }}>CRYSTAL · FANTASY · MMO</div>
            <div style={{ display: "flex", gap: 28, marginTop: 6 }}>
              {[["18K+","Online"],["500+","Worlds"],["1M+","Players"]].map(([n,l]) => (
                <div key={l} style={{ textAlign: "center" }}>
                  <div style={{ ...pxFont, fontSize: 13, color: "#4DE1FF" }}>{n}</div>
                  <div style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>{l}</div>
                </div>
              ))}
            </div>
            {/* Player flow hint */}
            <div style={{ marginTop: 12, padding: "12px 16px", background: "rgba(44,49,69,0.60)", border: "1px solid rgba(106,127,181,0.25)", borderRadius: 8, maxWidth: 280 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#4DE1FF", marginBottom: 8 }}>PLAYER FLOW</div>
              {["Login","Authenticating","Connecting","Continue / World Select","Spawn into World"].map((step, i) => (
                <div key={i} style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: i < 4 ? 4 : 0 }}>
                  <div style={{ width: 16, height: 16, borderRadius: 4, background: "rgba(79,140,255,0.20)", border: "1px solid rgba(79,140,255,0.40)", display: "flex", alignItems: "center", justifyContent: "center" }}>
                    <span style={{ ...pxFont, fontSize: 6, color: "#4DE1FF" }}>{i + 1}</span>
                  </div>
                  <span style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>{step}</span>
                </div>
              ))}
            </div>
          </div>
          {/* Right login form */}
          <div style={{ width: 304, display: "flex", alignItems: "center", justifyContent: "center", padding: 24, borderLeft: "1px solid rgba(106,127,181,0.22)" }}>
            <CrystalPanel style={{ padding: 24, width: "100%", animation: "fadeInScale .3s ease-out" }}>
              <div style={{ ...pxFont, fontSize: 10, color: "#fff", textAlign: "center", marginBottom: 20 }}>LOGIN</div>
              <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
                <div>
                  <label style={{ ...pxFont, fontSize: 7, color: "#C7D0E0", display: "block", marginBottom: 5 }}>USERNAME OR EMAIL</label>
                  <input className="sv-input" defaultValue="CrystalMage_42" readOnly/>
                </div>
                <div>
                  <label style={{ ...pxFont, fontSize: 7, color: "#C7D0E0", display: "block", marginBottom: 5 }}>PASSWORD</label>
                  <input className="sv-input" type="password" defaultValue="password" readOnly/>
                </div>
                <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
                  <label style={{ display: "flex", alignItems: "center", gap: 6, cursor: "pointer" }}>
                    <input type="checkbox" defaultChecked style={{ accentColor: "#4F8CFF" }}/>
                    <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>Remember me</span>
                  </label>
                  <span style={{ ...vtFont, fontSize: 13, color: "#4DE1FF", cursor: "pointer" }}>Forgot password?</span>
                </div>
                <Btn style={{ width: "100%", padding: "11px" }}>ENTER WORLD</Btn>
                <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
                  <div style={{ flex: 1, height: 1, background: "rgba(106,127,181,0.25)" }}/>
                  <span style={{ ...vtFont, fontSize: 12, color: "#6A7FB5" }}>new player?</span>
                  <div style={{ flex: 1, height: 1, background: "rgba(106,127,181,0.25)" }}/>
                </div>
                <Btn variant="purple" style={{ width: "100%", padding: "9px", fontSize: 8 }}>CREATE ACCOUNT</Btn>
              </div>
            </CrystalPanel>
          </div>
        </div>
      )}

      {/* ── REGISTER ── */}
      {screen === "register" && (
        <div style={{ height: 460, borderRadius: 10, background: "linear-gradient(135deg,#090e1a 0%,#101830 100%)", display: "flex", overflow: "hidden" }}>
          <div className="sv-pixel-grid" style={{ flex: 1, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 16, padding: 32 }}>
            <div style={{ ...pxFont, fontSize: 24, color: "#fff", textShadow: "0 0 24px rgba(108,92,231,0.80), 3px 3px 0 #000" }}>STRIXVERSE</div>
            <div style={{ ...pxFont, fontSize: 8, color: "#6C5CE7", letterSpacing: "0.14em" }}>JOIN THE CRYSTAL WORLD</div>
            {/* Appearance philosophy notice */}
            <div style={{ marginTop: 16, padding: "14px 18px", background: "rgba(108,92,231,0.10)", border: "1px solid rgba(108,92,231,0.35)", borderRadius: 10, maxWidth: 300 }}>
              <div style={{ ...pxFont, fontSize: 7, color: "#6C5CE7", marginBottom: 10 }}>YOUR LOOK = YOUR JOURNEY</div>
              <div style={{ ...vtFont, fontSize: 15, color: "#C7D0E0", lineHeight: 1.5 }}>
                There is no character creation screen in StrixVerse.
              </div>
              <div style={{ ...vtFont, fontSize: 15, color: "#C7D0E0", lineHeight: 1.5, marginTop: 6 }}>
                Your appearance evolves as you play — through gear you earn, craft, trade, and collect from quests and events.
              </div>
              <div style={{ marginTop: 12, display: "flex", flex: 1, gap: 8 }}>
                {["⚔️ Equip","🧪 Craft","🏪 Trade","🎯 Quest"].map(t => (
                  <div key={t} style={{ ...vtFont, fontSize: 12, padding: "3px 8px", borderRadius: 4, background: "rgba(108,92,231,0.18)", border: "1px solid rgba(108,92,231,0.35)", color: "#6C5CE7" }}>{t}</div>
                ))}
              </div>
            </div>
          </div>
          <div style={{ width: 320, display: "flex", alignItems: "center", justifyContent: "center", padding: 24, borderLeft: "1px solid rgba(106,127,181,0.22)" }}>
            <CrystalPanel style={{ padding: 24, width: "100%", animation: "fadeInScale .3s ease-out" }}>
              <div style={{ ...pxFont, fontSize: 10, color: "#fff", textAlign: "center", marginBottom: 18 }}>CREATE ACCOUNT</div>
              <div style={{ display: "flex", flexDirection: "column", gap: 11 }}>
                <div>
                  <label style={{ ...pxFont, fontSize: 7, color: "#C7D0E0", display: "block", marginBottom: 4 }}>USERNAME</label>
                  <input className="sv-input" placeholder="Choose a unique name" readOnly/>
                </div>
                <div>
                  <label style={{ ...pxFont, fontSize: 7, color: "#C7D0E0", display: "block", marginBottom: 4 }}>EMAIL ADDRESS</label>
                  <input className="sv-input" placeholder="your@email.com" readOnly/>
                </div>
                <div>
                  <label style={{ ...pxFont, fontSize: 7, color: "#C7D0E0", display: "block", marginBottom: 4 }}>PASSWORD</label>
                  <input className="sv-input" type="password" placeholder="Min 8 characters" readOnly/>
                </div>
                <div>
                  <label style={{ ...pxFont, fontSize: 7, color: "#C7D0E0", display: "block", marginBottom: 4 }}>CONFIRM PASSWORD</label>
                  <input className="sv-input" type="password" placeholder="Repeat password" readOnly/>
                </div>
                <label style={{ display: "flex", alignItems: "flex-start", gap: 8, cursor: "pointer" }}>
                  <input type="checkbox" defaultChecked style={{ accentColor: "#6C5CE7", marginTop: 2 }}/>
                  <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", lineHeight: 1.4 }}>I agree to the Terms of Service and Privacy Policy</span>
                </label>
                <Btn variant="purple" style={{ width: "100%", padding: "11px" }}>CREATE ACCOUNT</Btn>
                <div style={{ textAlign: "center" }}>
                  <span style={{ ...vtFont, fontSize: 13, color: "#4DE1FF", cursor: "pointer" }}>Already have an account? Sign in</span>
                </div>
              </div>
            </CrystalPanel>
          </div>
        </div>
      )}

      {/* ── CONNECTING ── */}
      {screen === "connecting" && (
        <div className="sv-pixel-grid" style={{ height: 460, borderRadius: 10, background: "linear-gradient(180deg,#090e1a 0%,#0f1828 100%)", display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 28, position: "relative" }}>
          <div style={{ position: "absolute", inset: 0, backgroundImage: "radial-gradient(ellipse at 50% 40%, rgba(77,225,255,0.07) 0%, transparent 65%)", borderRadius: 10 }}/>
          {/* Animated crystal icon */}
          <div style={{ animation: "crystalPulse 1.6s ease-in-out infinite", width: 56, height: 56, borderRadius: 16, background: "linear-gradient(135deg,#4F8CFF22,#6C5CE722)", border: "1px solid rgba(77,225,255,0.30)", display: "flex", alignItems: "center", justifyContent: "center", fontSize: 28 }}>
            💎
          </div>
          <div style={{ textAlign: "center" }}>
            <div style={{ ...pxFont, fontSize: 13, color: "#fff", textShadow: "0 0 20px rgba(77,225,255,0.5)", marginBottom: 8 }}>ENTERING STRIXVERSE</div>
            <div style={{ ...vtFont, fontSize: 18, color: "#4CD964" }}>Authentication complete ✓</div>
          </div>
          {/* All steps done — last world check resolves */}
          <div style={{ display: "flex", flexDirection: "column", gap: 8, minWidth: 280 }}>
            {[
              { label:"Authenticating",         done: true },
              { label:"Connecting",             done: true },
              { label:"Loading Player Data",    done: true },
              { label:"Checking Last World",    done: true },
            ].map((step, i) => (
              <div key={i} style={{ display: "flex", alignItems: "center", gap: 12 }}>
                <div style={{ width: 18, height: 18, borderRadius: "50%", border: "1px solid #4CD964", background: "rgba(76,217,100,0.15)", display: "flex", alignItems: "center", justifyContent: "center", flexShrink: 0 }}>
                  <span style={{ fontSize: 9, color: "#4CD964" }}>✓</span>
                </div>
                <span style={{ ...vtFont, fontSize: 16, color: "#C7D0E0" }}>{step.label}</span>
              </div>
            ))}
          </div>
          {/* Branch outcome */}
          <div style={{ padding: "14px 20px", background: "rgba(79,140,255,0.07)", border: "1px solid rgba(79,140,255,0.25)", borderRadius: 10, textAlign: "center", minWidth: 280 }}>
            <div style={{ ...vtFont, fontSize: 15, color: "#4DE1FF", marginBottom: 10 }}>Last world found — where would you like to go?</div>
            <div style={{ display: "flex", gap: 10 }}>
              <Btn style={{ flex: 1 }} onClick={() => setScreen("continue")}>▶ CONTINUE</Btn>
              <Btn variant="purple" style={{ flex: 1 }} onClick={() => setScreen("worldselect")}>CHANGE WORLD</Btn>
            </div>
          </div>
          <div style={{ position: "absolute", bottom: 18, textAlign: "center" }}>
            <span style={{ ...vtFont, fontSize: 15, color: "#6A7FB5", fontStyle: "italic" }}>
              "The crystal nexus hums with ancient power..."
            </span>
          </div>
        </div>
      )}

      {/* ── CONTINUE (last world) ── */}
      {screen === "continue" && (
        <div className="sv-pixel-grid" style={{ height: 460, borderRadius: 10, background: "linear-gradient(180deg,#090e1a 0%,#0f1828 100%)", display: "flex", alignItems: "center", justifyContent: "center", position: "relative", overflow: "hidden" }}>
          <div style={{ position: "absolute", inset: 0, backgroundImage: "radial-gradient(ellipse at 50% 40%, rgba(76,217,100,0.06) 0%, transparent 60%)", borderRadius: 10 }}/>
          {/* Decorative crystal shards */}
          {[[8,12,18],[88,8,14],[4,72,12],[91,70,16],[50,88,10]].map(([l,t,sz],i) => (
            <div key={i} style={{ position:"absolute", left:`${l}%`, top:`${t}%`, width: sz, height: sz * 2, background: "rgba(77,225,255,0.05)", transform: "rotate(30deg)", borderRadius: 2, border:"1px solid rgba(77,225,255,0.10)" }}/>
          ))}
          <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 24, animation: "fadeInScale .35s ease-out", zIndex: 1 }}>
            {/* Welcome banner */}
            <div style={{ textAlign: "center" }}>
              <div style={{ ...vtFont, fontSize: 22, color: "#C7D0E0" }}>Welcome back,</div>
              <div style={{ ...pxFont, fontSize: 16, color: "#fff", textShadow: "0 0 20px rgba(77,225,255,0.5)", marginTop: 4 }}>CrystalMage_42</div>
            </div>
            {/* Last world card */}
            <CrystalPanel style={{ width: 360, padding: 0, overflow: "hidden" }}>
              <div style={{ background: "linear-gradient(180deg, rgba(76,217,100,0.08) 0%, transparent 100%)", borderBottom: "1px solid rgba(76,217,100,0.20)", padding: "14px 20px", display: "flex", alignItems: "center", gap: 12 }}>
                <div style={{ width: 40, height: 40, borderRadius: 10, background: "linear-gradient(135deg,#4CD96422,#4F8CFF22)", border: "1px solid rgba(76,217,100,0.30)", display: "flex", alignItems: "center", justifyContent: "center", fontSize: 20, flexShrink: 0 }}>🌿</div>
                <div>
                  <div style={{ ...pxFont, fontSize: 6, color: "#4CD964", marginBottom: 3 }}>LAST VISITED WORLD</div>
                  <div style={{ ...pxFont, fontSize: 11, color: "#fff" }}>Green Valley</div>
                </div>
                <div style={{ marginLeft: "auto", display: "flex", alignItems: "center", gap: 5 }}>
                  <div style={{ width: 7, height: 7, borderRadius: "50%", background: "#4CD964", boxShadow: "0 0 6px #4CD964" }}/>
                  <span style={{ ...vtFont, fontSize: 14, color: "#4CD964" }}>ONLINE</span>
                </div>
              </div>
              <div style={{ padding: "16px 20px", display: "flex", flexDirection: "column", gap: 8 }}>
                {[
                  { label:"World Type",   val:"Survival",    icon:"⚔️" },
                  { label:"Owner",        val:"CrystalMage_42", icon:"👤" },
                  { label:"Last Played",  val:"2 hours ago", icon:"🕐" },
                  { label:"Position",     val:"X:1842 Y:64", icon:"📍" },
                  { label:"Players",      val:"3 / 50 online",icon:"👥" },
                ].map(r => (
                  <div key={r.label} style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
                    <div style={{ display: "flex", alignItems: "center", gap: 7 }}>
                      <span style={{ fontSize: 11 }}>{r.icon}</span>
                      <span style={{ ...vtFont, fontSize: 15, color: "#6A7FB5" }}>{r.label}</span>
                    </div>
                    <span style={{ ...monoFont, fontSize: 10, color: "#C7D0E0" }}>{r.val}</span>
                  </div>
                ))}
              </div>
              <div style={{ padding: "0 20px 20px", display: "flex", gap: 10 }}>
                <Btn style={{ flex: 1, padding: "12px" }} variant="primary">▶ CONTINUE</Btn>
                <Btn variant="purple" style={{ padding: "12px 16px", fontSize: 8 }} onClick={() => setScreen("worldselect")}>CHANGE WORLD</Btn>
              </div>
            </CrystalPanel>
            <span style={{ ...vtFont, fontSize: 14, color: "#6A7FB5" }}>Auto-joining in <span style={{ color: "#4DE1FF" }}>5</span>s — press any key to cancel</span>
          </div>
        </div>
      )}

      {/* ── WORLD SELECTION ── */}
      {screen === "worldselect" && (() => {
        const WORLDS = [
          { name:"Green Valley",    owner:"CrystalMage_42", type:"Survival",  players:3,  max:50,  desc:"Lush starter biome with abundant resources.",          fav:true,  recent:true,  tag:"RECENT"   },
          { name:"Crystal Caverns", owner:"NexusForge",     type:"Adventure", players:18, max:50,  desc:"Deep underground crystal labyrinth. Watch your step.", fav:true,  recent:false, tag:"FAVORITE" },
          { name:"SkyCity Alpha",   owner:"PixelMage",      type:"Creative",  players:41, max:50,  desc:"Floating islands above the clouds.",                   fav:false, recent:true,  tag:"RECENT"   },
          { name:"Void Frontier",   owner:"ShadowLord",     type:"Survival",  players:7,  max:50,  desc:"Harsh void biome. For experienced players only.",      fav:false, recent:false, tag:""         },
          { name:"The Grand Bazaar",owner:"TradeMaster",    type:"Trading",   players:50, max:50,  desc:"Largest player marketplace in StrixVerse.",            fav:true,  recent:false, tag:"FAVORITE" },
          { name:"Event Grounds",   owner:"[Staff]",        type:"Event",     players:28, max:200, desc:"Crystal Festival 2025 — Limited time world!",          fav:false, recent:false, tag:"EVENT"    },
        ];
        const filtered = WORLDS.filter(w => {
          const matchQ = w.name.toLowerCase().includes(wsQuery.toLowerCase()) || w.owner.toLowerCase().includes(wsQuery.toLowerCase());
          if (!matchQ) return false;
          if (wsFilter === "recent") return w.recent;
          if (wsFilter === "favorites") return w.fav;
          return true;
        });
        const tagColor: Record<string,string> = { RECENT:"#4F8CFF", FAVORITE:"#FFD700", EVENT:"#FF5A5A", "":"transparent" };
        return (
          <div style={{ height: 460, borderRadius: 10, background: "#0e1424", border: "1px solid rgba(106,127,181,0.28)", display: "flex", flexDirection: "column", overflow: "hidden" }}>
            {/* Header */}
            <div style={{ padding: "16px 20px", borderBottom: "1px solid rgba(106,127,181,0.20)", display: "flex", alignItems: "center", gap: 14 }}>
              <div style={{ flex: 1 }}>
                <div style={{ ...pxFont, fontSize: 11, color: "#fff", marginBottom: 2 }}>WORLD SELECTION</div>
                <div style={{ ...vtFont, fontSize: 14, color: "#C7D0E0" }}>Choose a world to enter</div>
              </div>
              {/* Search */}
              <input
                className="sv-input"
                placeholder="Search worlds..."
                value={wsQuery}
                onChange={e => setWsQuery(e.target.value)}
                style={{ width: 200, ...vtFont, fontSize: 15, padding: "6px 12px" }}
              />
              {/* Filter pills */}
              <div style={{ display: "flex", gap: 5 }}>
                {(["all","recent","favorites"] as const).map(f => (
                  <button key={f} onClick={() => setWsFilter(f)}
                    style={{ ...vtFont, fontSize: 13, padding: "5px 12px", borderRadius: 5, cursor: "pointer", border: wsFilter === f ? "1px solid rgba(79,140,255,0.50)" : "1px solid rgba(106,127,181,0.20)", background: wsFilter === f ? "rgba(79,140,255,0.18)" : "transparent", color: wsFilter === f ? "#4DE1FF" : "#C7D0E0", transition: "all .12s" }}>
                    {f === "all" ? "All" : f === "recent" ? "⏰ Recent" : "⭐ Favorites"}
                  </button>
                ))}
              </div>
              <Btn variant="purple" size="sm">↺ Refresh</Btn>
            </div>
            {/* World list */}
            <div className="sv-scroll" style={{ flex: 1, overflowY: "auto", padding: 14, display: "flex", flexDirection: "column", gap: 8 }}>
              {filtered.length === 0 && (
                <div style={{ textAlign: "center", padding: 40 }}>
                  <div style={{ fontSize: 32, marginBottom: 8 }}>🔍</div>
                  <div style={{ ...vtFont, fontSize: 18, color: "#6A7FB5" }}>No worlds match your search.</div>
                </div>
              )}
              {filtered.map((w, i) => {
                const pct = Math.round((w.players / w.max) * 100);
                const popCol = pct >= 100 ? "#FF5A5A" : pct >= 80 ? "#FFD54A" : "#4CD964";
                const typeColor: Record<string,string> = { Survival:"#4CD964", Adventure:"#4F8CFF", Creative:"#6C5CE7", Trading:"#FFD700", Event:"#FF5A5A" };
                return (
                  <div key={i} style={{ display: "flex", alignItems: "center", gap: 14, padding: "12px 16px", borderRadius: 9, border: w.recent ? "1px solid rgba(79,140,255,0.35)" : "1px solid rgba(106,127,181,0.20)", background: w.recent ? "rgba(79,140,255,0.06)" : "rgba(14,18,30,0.50)", cursor: "pointer", transition: "all .12s" }}
                    onMouseEnter={e => { (e.currentTarget as HTMLElement).style.borderColor = "rgba(77,225,255,0.45)"; (e.currentTarget as HTMLElement).style.background = "rgba(77,225,255,0.06)"; }}
                    onMouseLeave={e => { (e.currentTarget as HTMLElement).style.borderColor = w.recent ? "rgba(79,140,255,0.35)" : "rgba(106,127,181,0.20)"; (e.currentTarget as HTMLElement).style.background = w.recent ? "rgba(79,140,255,0.06)" : "rgba(14,18,30,0.50)"; }}>
                    {/* Fav indicator */}
                    <span style={{ fontSize: 14, opacity: w.fav ? 1 : 0.2, flexShrink: 0 }}>⭐</span>
                    {/* Main info */}
                    <div style={{ flex: 1, minWidth: 0 }}>
                      <div style={{ display: "flex", alignItems: "center", gap: 8, flexWrap: "wrap" }}>
                        <span style={{ ...pxFont, fontSize: 8, color: "#fff" }}>{w.name}</span>
                        {w.tag && <span style={{ ...pxFont, fontSize: 5, color: tagColor[w.tag], padding: "2px 6px", background: `${tagColor[w.tag]}18`, border: `1px solid ${tagColor[w.tag]}44`, borderRadius: 3 }}>{w.tag}</span>}
                        <span style={{ ...vtFont, fontSize: 12, color: typeColor[w.type] ?? "#C7D0E0", padding: "1px 7px", background: `${typeColor[w.type] ?? "#C7D0E0"}18`, borderRadius: 3 }}>{w.type}</span>
                      </div>
                      <div style={{ ...vtFont, fontSize: 13, color: "#6A7FB5", marginTop: 2 }}>by {w.owner}</div>
                      <div style={{ ...vtFont, fontSize: 13, color: "#C7D0E0", marginTop: 3, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{w.desc}</div>
                    </div>
                    {/* Population */}
                    <div style={{ textAlign: "right", flexShrink: 0, minWidth: 80 }}>
                      <div style={{ ...monoFont, fontSize: 10, color: popCol }}>{w.players}/{w.max}</div>
                      <div className="sv-bar" style={{ width: 80, height: 4, marginTop: 4 }}>
                        <div className="sv-bar-fill" style={{ width: `${pct}%`, background: popCol, borderRadius: 2 }}/>
                      </div>
                      <Btn size="sm" variant={pct >= 100 ? "disabled" : "primary"} style={{ marginTop: 6, fontSize: 7, padding: "5px 10px" }}>
                        {pct >= 100 ? "FULL" : "JOIN"}
                      </Btn>
                    </div>
                  </div>
                );
              })}
            </div>
            {/* Footer */}
            <div style={{ padding: "10px 20px", borderTop: "1px solid rgba(106,127,181,0.15)", display: "flex", alignItems: "center", justifyContent: "space-between" }}>
              <span style={{ ...vtFont, fontSize: 14, color: "#6A7FB5" }}>{WORLDS.length} worlds available</span>
              <Btn variant="purple" size="sm" onClick={() => setScreen("continue")}>← BACK</Btn>
            </div>
          </div>
        );
      })()}

      {/* ── LOADING ── */}
      {screen === "loading" && (
        <div className="sv-pixel-grid" style={{ height: 460, borderRadius: 10, background: "linear-gradient(180deg,#090e1a 0%,#0f1828 100%)", display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 28, position: "relative" }}>
          <div style={{ textAlign: "center" }}>
            <div style={{ ...pxFont, fontSize: 12, color: "#fff", textShadow: "0 0 20px rgba(77,225,255,0.5)" }}>ENTERING WORLD</div>
            <div style={{ ...vtFont, fontSize: 20, color: "#4DE1FF", marginTop: 6 }}>Loading World...</div>
          </div>
          <div style={{ width: 360 }}>
            <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
              <span style={{ ...vtFont, fontSize: 15, color: "#C7D0E0" }}>Loading world data...</span>
              <span style={{ ...monoFont, fontSize: 11, color: "#4DE1FF" }}>73%</span>
            </div>
            <div className="sv-bar" style={{ height: 14, borderRadius: 7 }}>
              <div className="sv-bar-fill" style={{ width: "73%", background: "linear-gradient(90deg,#4F8CFF,#4DE1FF)", boxShadow: "0 0 12px rgba(77,225,255,0.65)", borderRadius: 7 }}/>
            </div>
            <div style={{ marginTop: 10, display: "flex", flexDirection: "column", gap: 3 }}>
              {[
                { label:"World tiles",    done:true  },
                { label:"Entity data",    done:true  },
                { label:"Player session", done:true  },
                { label:"Crystal shards", done:false },
              ].map(step => (
                <div key={step.label} style={{ display: "flex", alignItems: "center", gap: 8 }}>
                  <span style={{ fontSize: 10, color: step.done ? "#4CD964" : "#6A7FB5" }}>{step.done ? "✓" : "○"}</span>
                  <span style={{ ...vtFont, fontSize: 14, color: step.done ? "#C7D0E0" : "#6A7FB5" }}>{step.label}</span>
                </div>
              ))}
            </div>
          </div>
          <div style={{ display: "flex", gap: 6 }}>
            {[0,1,2,3,4].map(i => (
              <div key={i} style={{ width: 8, height: 8, borderRadius: "50%", background: i < 4 ? "#4F8CFF" : "rgba(106,127,181,0.25)", boxShadow: i < 4 ? "0 0 6px #4F8CFF" : "none", animation: `glowPulse ${.5 + i * .15}s ease-in-out infinite` }}/>
            ))}
          </div>
          <div style={{ position: "absolute", bottom: 14, left: "50%", transform: "translateX(-50%)", width: 420, padding: "10px 16px", borderRadius: 8, background: "rgba(14,18,30,0.75)", border: "1px solid rgba(77,225,255,0.20)", backdropFilter: "blur(6px)", textAlign: "center" }}>
            <span style={{ ...pxFont, fontSize: 5, color: "#4DE1FF", display: "block", marginBottom: 4 }}>💡 LOADING TIP</span>
            <span style={{ ...vtFont, fontSize: 15, color: "#C7D0E0", lineHeight: 1.5 }}>{LOADING_TIPS[tipIdx]}</span>
          </div>
        </div>
      )}

      {/* ── PAUSE MENU ── */}
      {screen === "pause" && (
        <div style={{ height: 460, borderRadius: 10, position: "relative", overflow: "hidden" }}>
          {/* Blurred game world behind */}
          <div className="sv-pixel-grid" style={{ position: "absolute", inset: 0, background: "linear-gradient(180deg,#0d1620 0%,#182535 45%,#0f1a22 100%)", filter: "blur(2px)", opacity: 0.6 }}/>
          <div style={{ position: "absolute", inset: 0, background: "rgba(10,14,24,0.65)" }}/>
          <div style={{ position: "relative", zIndex: 1, display: "flex", alignItems: "center", justifyContent: "center", height: "100%" }}>
            <CrystalPanel style={{ width: 280, padding: 28, animation: "fadeInScale .25s ease-out" }}>
              <div style={{ ...pxFont, fontSize: 12, color: "#fff", textAlign: "center", marginBottom: 6 }}>PAUSED</div>
              <div style={{ ...vtFont, fontSize: 14, color: "#C7D0E0", textAlign: "center", marginBottom: 24 }}>Session active</div>
              <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
                {[
                  { label:"RESUME",        variant:"primary" as const  },
                  { label:"SETTINGS",      variant:"purple" as const   },
                  { label:"INVENTORY",     variant:"primary" as const  },
                  { label:"QUIT TO MENU",  variant:"danger" as const   },
                ].map(opt => (
                  <Btn key={opt.label} variant={opt.variant} style={{ width: "100%", padding: "11px" }}>{opt.label}</Btn>
                ))}
              </div>
              <div style={{ marginTop: 20, padding: "10px 14px", background: "rgba(14,18,30,0.65)", borderRadius: 7, border: "1px solid rgba(106,127,181,0.20)" }}>
                <div style={{ display: "flex", justifyContent: "space-between" }}>
                  <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>Session time</span>
                  <span style={{ ...monoFont, fontSize: 10, color: "#4DE1FF" }}>2h 14m</span>
                </div>
                <div style={{ display: "flex", justifyContent: "space-between", marginTop: 4 }}>
                  <span style={{ ...vtFont, fontSize: 13, color: "#C7D0E0" }}>Ping</span>
                  <span style={{ ...monoFont, fontSize: 10, color: "#4CD964" }}>24 ms</span>
                </div>
              </div>
            </CrystalPanel>
          </div>
        </div>
      )}

      {/* ── SETTINGS ── */}
      {screen === "settings" && (() => {
        const SETTING_CATS = ["GENERAL","GRAPHICS","AUDIO","CONTROLS","ACCOUNT","ACCESSIBILITY"] as const;
        return (
          <div style={{ height: 460, borderRadius: 10, background: "#0e1424", border: "1px solid rgba(106,127,181,0.28)", display: "flex", overflow: "hidden" }}>
            {/* Sidebar */}
            <div style={{ width: 160, borderRight: "1px solid rgba(106,127,181,0.20)", display: "flex", flexDirection: "column", padding: "16px 0" }}>
              <div style={{ ...pxFont, fontSize: 8, color: "#6A7FB5", padding: "0 16px 12px" }}>SETTINGS</div>
              {SETTING_CATS.map((cat, i) => (
                <button key={cat} style={{ ...vtFont, fontSize: 15, textAlign: "left", padding: "9px 16px", color: i === 0 ? "#4DE1FF" : "#C7D0E0", background: i === 0 ? "rgba(79,140,255,0.12)" : "transparent", borderLeft: i === 0 ? "2px solid #4F8CFF" : "2px solid transparent", cursor: "pointer", transition: "all .12s" }}
                  onMouseEnter={e => { if (i !== 0) { (e.currentTarget as HTMLElement).style.background = "rgba(79,140,255,0.06)"; (e.currentTarget as HTMLElement).style.color = "#fff"; }}}
                  onMouseLeave={e => { if (i !== 0) { (e.currentTarget as HTMLElement).style.background = "transparent"; (e.currentTarget as HTMLElement).style.color = "#C7D0E0"; }}}>
                  {cat}
                </button>
              ))}
              <div style={{ flex: 1 }}/>
              <div style={{ padding: "0 12px" }}>
                <Btn variant="danger" size="sm" style={{ width: "100%" }}>RESET ALL</Btn>
              </div>
            </div>
            {/* Settings content */}
            <div className="sv-scroll" style={{ flex: 1, padding: 20, overflowY: "auto" }}>
              <div style={{ ...pxFont, fontSize: 9, color: "#4DE1FF", marginBottom: 16 }}>GENERAL</div>
              <div style={{ display: "flex", flexDirection: "column", gap: 4 }}>
                {[
                  { label:"Username",     val:"CrystalMage_42",  type:"text"   },
                  { label:"Language",     val:"English",          type:"select" },
                  { label:"Interface Scale", val:"100%",          type:"select" },
                  { label:"Show FPS",     val:true,               type:"toggle" },
                  { label:"Show Ping",    val:true,               type:"toggle" },
                  { label:"Chat Filter",  val:false,              type:"toggle" },
                  { label:"Auto-save",    val:true,               type:"toggle" },
                  { label:"Notification Sound", val:true,         type:"toggle" },
                ].map(s => (
                  <div key={s.label} style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "10px 14px", borderRadius: 7, border: "1px solid rgba(106,127,181,0.16)", background: "rgba(14,18,30,0.45)" }}>
                    <span style={{ ...vtFont, fontSize: 15, color: "#C7D0E0" }}>{s.label}</span>
                    {s.type === "toggle" ? (
                      <div style={{ width: 36, height: 20, borderRadius: 10, background: (s.val as boolean) ? "#4F8CFF" : "#3A4060", border: `1px solid ${(s.val as boolean) ? "rgba(79,140,255,0.6)" : "rgba(106,127,181,0.3)"}`, position: "relative", cursor: "pointer", boxShadow: (s.val as boolean) ? "0 0 8px rgba(79,140,255,0.4)" : "none", transition: "all .2s" }}>
                        <div style={{ width: 14, height: 14, borderRadius: "50%", background: "#fff", position: "absolute", top: 2, left: (s.val as boolean) ? 19 : 2, transition: "left .2s", boxShadow: "0 1px 3px rgba(0,0,0,0.4)" }}/>
                      </div>
                    ) : s.type === "select" ? (
                      <div style={{ ...vtFont, fontSize: 14, color: "#4DE1FF", padding: "3px 10px", background: "rgba(79,140,255,0.10)", border: "1px solid rgba(79,140,255,0.30)", borderRadius: 5, cursor: "pointer" }}>{s.val as string} ▾</div>
                    ) : (
                      <div style={{ ...vtFont, fontSize: 14, color: "#fff", padding: "3px 10px", background: "rgba(44,49,69,0.6)", border: "1px solid rgba(106,127,181,0.25)", borderRadius: 5 }}>{s.val as string}</div>
                    )}
                  </div>
                ))}
              </div>
              <div style={{ marginTop: 16, display: "flex", gap: 8 }}>
                <Btn style={{ flex: 1 }}>SAVE CHANGES</Btn>
                <Btn variant="purple" size="sm">APPLY</Btn>
              </div>
            </div>
          </div>
        );
      })()}

      {/* ── CREDITS ── */}
      {screen === "credits" && (
        <div className="sv-pixel-grid" style={{ height: 460, borderRadius: 10, background: "linear-gradient(180deg,#090e1a 0%,#0d1525 100%)", overflow: "hidden", display: "flex", flexDirection: "column", alignItems: "center", padding: 28, gap: 0 }}>
          <div style={{ ...pxFont, fontSize: 14, color: "#fff", textShadow: "0 0 20px rgba(77,225,255,0.6)", marginBottom: 4 }}>STRIXVERSE</div>
          <div style={{ ...vtFont, fontSize: 14, color: "#4DE1FF", marginBottom: 24 }}>Crystal Technology · Fantasy World</div>
          <div className="sv-scroll" style={{ width: "100%", maxWidth: 560, overflowY: "auto", flex: 1 }}>
            {[
              { role:"GAME DIRECTOR",     name:"V. Aldric",         note:"" },
              { role:"LEAD DEVELOPER",    name:"A. Strix",          note:"" },
              { role:"UI / UX DESIGNER",  name:"Crystal OS Team",   note:"" },
              { role:"GAME ARTIST",       name:"Pixel Forge Studio",note:"" },
              { role:"MUSIC & SFX",       name:"Echo Sound Lab",    note:"" },
              { role:"WORLD DESIGN",      name:"Terracraft Guild",  note:"" },
              { role:"QUALITY ASSURANCE", name:"Crystal Testers",   note:"" },
              { role:"COMMUNITY",         name:"StrixVerse Players", note:"Thank you for playing!" },
            ].map((c, i) => (
              <div key={i} style={{ display: "flex", justifyContent: "space-between", alignItems: "center", padding: "10px 16px", borderBottom: "1px solid rgba(106,127,181,0.12)" }}>
                <div>
                  <div style={{ ...pxFont, fontSize: 7, color: "#6A7FB5" }}>{c.role}</div>
                  <div style={{ ...vtFont, fontSize: 16, color: "#fff", marginTop: 2 }}>{c.name}</div>
                </div>
                {c.note && <span style={{ ...vtFont, fontSize: 13, color: "#4DE1FF" }}>{c.note}</span>}
              </div>
            ))}
            <div style={{ textAlign: "center", padding: "20px 0" }}>
              <div style={{ ...pxFont, fontSize: 8, color: "#6A7FB5" }}>SPECIAL THANKS</div>
              <div style={{ ...vtFont, fontSize: 15, color: "#C7D0E0", marginTop: 8, lineHeight: 1.6 }}>
                Every player who explored, crafted, traded, and helped<br/>
                build the crystal world from the very first day.
              </div>
              <div style={{ ...monoFont, fontSize: 10, color: "#4DE1FF", marginTop: 12 }}>
                StrixVerse © 2025 — All Rights Reserved
              </div>
            </div>
          </div>
        </div>
      )}

      {/* ── ERROR / CONNECTION LOST ── */}
      {screen === "error" && (
        <div style={{ height: 460, borderRadius: 10, background: "linear-gradient(180deg,#140608 0%,#1a0a0a 100%)", display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 20, position: "relative" }}>
          <div style={{ position: "absolute", inset: 0, backgroundImage: "radial-gradient(ellipse at 50% 40%, rgba(255,90,90,0.08) 0%, transparent 65%)", borderRadius: 10 }}/>
          <div style={{ animation: "glowPulse 1.2s ease-in-out infinite" }}>
            <div style={{ fontSize: 56, textAlign: "center" }}>📡</div>
          </div>
          <div style={{ textAlign: "center" }}>
            <div style={{ ...pxFont, fontSize: 14, color: "#FF5A5A", textShadow: "0 0 20px rgba(255,90,90,0.6)", marginBottom: 6 }}>CONNECTION LOST</div>
            <div style={{ ...vtFont, fontSize: 18, color: "#C7D0E0" }}>Unable to connect to the game server</div>
          </div>
          <CrystalPanel style={{ width: 340, padding: 18, border: "1px solid rgba(255,90,90,0.30)", boxShadow: "0 0 24px rgba(255,90,90,0.14)" }}>
            <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
              {[
                { label:"Error Code",   val:"ERR_TIMEOUT",    col:"#FF5A5A" },
                { label:"Status",      val:"Timed out",       col:"#FFD54A" },
                { label:"Session Time",val:"2h 14m",          col:"#C7D0E0" },
                { label:"Progress",    val:"Auto-saved ✓",    col:"#4CD964" },
              ].map(r => (
                <div key={r.label} style={{ display: "flex", justifyContent: "space-between" }}>
                  <span style={{ ...vtFont, fontSize: 14, color: "#6A7FB5" }}>{r.label}</span>
                  <span style={{ ...monoFont, fontSize: 10, color: r.col }}>{r.val}</span>
                </div>
              ))}
            </div>
            <div style={{ marginTop: 14, padding: "8px 0" }}>
              <div style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: 6 }}>
                <div style={{ width: 8, height: 8, borderRadius: "50%", background: "#FFD54A", animation: "glowPulse 0.8s infinite" }}/>
                <span style={{ ...vtFont, fontSize: 14, color: "#FFD54A" }}>Reconnecting... (3 / 5)</span>
              </div>
              <div className="sv-bar" style={{ height: 6 }}>
                <div className="sv-bar-fill" style={{ width: "60%", background: "#FFD54A", animation: "glowPulse 1s infinite" }}/>
              </div>
            </div>
          </CrystalPanel>
          <div style={{ display: "flex", gap: 10 }}>
            <Btn>RECONNECT NOW</Btn>
            <Btn variant="purple">RETURN TO LOGIN</Btn>
            <Btn variant="danger">QUIT GAME</Btn>
          </div>
        </div>
      )}
    </div>
  );
}

// ─── ROOT APP ─────────────────────────────────────────────────────────────────
export default function App() {
  const [tab, setTab] = useState<TabId>("overview");
  const [winSubTab, setWinSubTab] = useState<WinTab>("inventory");

  // Economy state — in production these values come from server packets only
  const [coins] = useState(52450);
  const [gems, setGems] = useState(1250);
  const [gemAnim, setGemAnim] = useState<string>("");
  const [floats, setFloats] = useState<FloatEntry[]>([]);
  const floatIdRef = React.useRef(0);

  function triggerGemChange(delta: number) {
    setGems(g => Math.max(0, g + delta));
    setGemAnim(delta > 0 ? "sv-gem-earn" : "sv-gem-spend");
    const id = ++floatIdRef.current;
    setFloats(f => [...f, { id, delta, x: 0, y: 0 }]);
    setTimeout(() => setGemAnim(""), 500);
    setTimeout(() => setFloats(f => f.filter(e => e.id !== id)), 1200);
  }

  function openWallet() { setTab("windows"); setWinSubTab("wallet"); }
  function openShop()   { setTab("windows"); setWinSubTab("shop");   }

  return (
    <>
      <style>{GLOBAL_CSS}</style>
      <div className="sv-pixel-grid" style={{ minHeight: "100vh", background: "#1E2230", fontFamily: "'VT323', monospace" }}>

        {/* ── HEADER ── */}
        <header style={{ background: "linear-gradient(180deg,#252a3f 0%,#1e2230 100%)", borderBottom: "1px solid rgba(106,127,181,0.28)", boxShadow: "0 2px 20px rgba(0,0,0,0.45)" }}>
          <div style={{ maxWidth: 1400, margin: "0 auto", padding: "0 24px", height: 60, display: "flex", alignItems: "center", justifyContent: "space-between" }}>
            <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
              <div style={{ width: 36, height: 36, borderRadius: 9, background: "linear-gradient(135deg,#4F8CFF,#6C5CE7)", display: "flex", alignItems: "center", justifyContent: "center", fontSize: 18, boxShadow: "0 0 16px rgba(77,225,255,0.45)" }}>
                💎
              </div>
              <div>
                <div style={{ ...pxFont, fontSize: 12, color: "#fff", textShadow: "0 0 12px rgba(77,225,255,0.35)" }}>STRIXVERSE</div>
                <div style={{ ...vtFont, fontSize: 13, color: "#4DE1FF", marginTop: -2 }}>UI STYLE GUIDE v1.0</div>
              </div>
            </div>
            <div style={{ display: "flex", alignItems: "center", gap: 20 }}>
              <div style={{ display: "flex", gap: 6 }}>
                {["#4F8CFF","#6C5CE7","#4DE1FF","#4CD964","#FFD700","#FF5A5A"].map(c => (
                  <div key={c} style={{ width: 10, height: 10, borderRadius: "50%", background: c, boxShadow: `0 0 6px ${c}` }}/>
                ))}
              </div>
              <span style={{ ...monoFont, fontSize: 10, color: "#C7D0E0" }}>Crystal Technology + Fantasy</span>
            </div>
          </div>
        </header>

        {/* ── TAB NAV ── */}
        <nav style={{ position: "sticky", top: 0, zIndex: 20, background: "rgba(22,26,40,0.96)", backdropFilter: "blur(10px)", borderBottom: "1px solid rgba(106,127,181,0.20)" }}>
          <div style={{ maxWidth: 1400, margin: "0 auto", padding: "0 24px", display: "flex", overflowX: "auto" }} className="sv-scroll">
            {TABS.map(t => (
              <button key={t.id} onClick={() => setTab(t.id)} className={`sv-tab ${tab === t.id ? "sv-tab-active" : ""}`}>
                {t.label}
              </button>
            ))}
          </div>
        </nav>

        {/* ── CONTENT ── */}
        <main style={{ maxWidth: 1400, margin: "0 auto", padding: "28px 24px 60px" }}>
          {tab === "overview"      && <OverviewTab/>}
          {tab === "buttons"       && <ButtonsTab/>}
          {tab === "hud"           && (
            <HUDTab
              coins={coins} gems={gems} gemAnim={gemAnim} floats={floats}
              onCoinsClick={openWallet} onGemsClick={openShop}
              triggerGemChange={triggerGemChange}
            />
          )}
          {tab === "windows"       && <WindowsTab subTab={winSubTab} setSubTab={setWinSubTab}/>}
          {tab === "icons"         && <IconsTab/>}
          {tab === "slots"         && <SlotsTab/>}
          {tab === "notifications" && <NotificationsTab/>}
          {tab === "screens"       && <ScreensTab/>}
        </main>

        {/* ── FOOTER ── */}
        <footer style={{ borderTop: "1px solid rgba(106,127,181,0.18)", padding: "14px 24px" }}>
          <div style={{ maxWidth: 1400, margin: "0 auto", display: "flex", justifyContent: "space-between", alignItems: "center" }}>
            <span style={{ ...monoFont, fontSize: 9, color: "rgba(199,208,224,0.35)" }}>StrixVerse UI Style Guide © 2025 · Crystal Technology + Fantasy</span>
            <div style={{ display: "flex", gap: 6, alignItems: "center" }}>
              <SvgIcon.Coins size={12} color="rgba(255,215,0,0.4)"/>
              <span style={{ ...monoFont, fontSize: 9, color: "rgba(199,208,224,0.35)" }}>Press Start 2P · VT323 · Share Tech Mono</span>
            </div>
          </div>
        </footer>
      </div>
    </>
  );
}
