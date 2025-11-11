export function normalizeVector(vector: number[]): number[] {
  if (vector.length === 0) return vector;
  const safe = vector.map((v) => (Number.isFinite(v) && v > 0 ? v : 0));
  const sum = safe.reduce((acc, value) => acc + value, 0);
  if (sum <= 0) {
    const uniform = 1 / vector.length;
    return vector.map(() => uniform);
  }
  return safe.map((v) => v / sum);
}

export function multiplyMatrixVector(matrix: number[][], vector: number[]): number[] {
  if (matrix.length === 0) return Array.from({ length: vector.length }, (_, idx) => vector[idx] ?? 0);
  return matrix.map((row) => {
    const padded = Array.from({ length: vector.length }, (_, idx) => row[idx] ?? 0);
    return padded.reduce((acc, value, idx) => acc + value * (vector[idx] ?? 0), 0);
  });
}

export function ensureSquareMatrix(matrix: number[][], size: number): number[][] {
  return Array.from({ length: size }, (_, rowIdx) =>
    Array.from({ length: size }, (_, colIdx) => matrix[rowIdx]?.[colIdx] ?? (rowIdx === colIdx ? 1 : 0))
  );
}

export function applyCapacity(Qin: number, throughputMax: number, lossFactor: number): number {
  const throughput = Math.max(throughputMax, 0);
  const losses = Math.min(Math.max(lossFactor, 0), 1);
  return Math.min(Math.max(Qin, 0), throughput) * (1 - losses);
}

export function distributeFlow(total: number, split: number[], outCount: number): number[] {
  if (outCount <= 0) return [];
  const base = Array.from({ length: outCount }, (_, idx) => split[idx] ?? 0);
  const normalized = normalizeVector(base.length ? base : Array(outCount).fill(1 / outCount));
  return normalized.map((ratio) => total * ratio);
}

export function padVector(vector: number[], size: number): number[] {
  return Array.from({ length: size }, (_, idx) => vector[idx] ?? 0);
}
