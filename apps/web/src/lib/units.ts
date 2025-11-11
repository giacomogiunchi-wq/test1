export const UNIT_FLOW = 'kg/h';

export function formatFlow(value: number) {
  if (!isFinite(value)) return `0 ${UNIT_FLOW}`;
  return `${value.toFixed(0)} ${UNIT_FLOW}`;
}

export function formatComposition(comp: number[]) {
  return comp.map((v) => `${(v * 100).toFixed(1)}%`).join(' / ');
}
