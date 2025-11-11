export function normalize(vector: number[]): number[] {
  const sum = vector.reduce((acc, value) => acc + value, 0);
  if (!isFinite(sum) || sum <= 0) {
    const uniform = 1 / vector.length;
    return vector.map(() => uniform);
  }
  return vector.map((v) => (v < 0 ? 0 : v) / sum);
}

export function multiplyMatrixVector(matrix: number[][], vector: number[]): number[] {
  if (matrix.length === 0) return vector;
  return matrix.map((row, rowIdx) => {
    const safeRow = row.length === vector.length ? row : new Array(vector.length).fill(0).map((_, idx) => row[idx] ?? 0);
    const sum = safeRow.reduce((acc, value, idx) => acc + value * vector[idx], 0);
    return sum;
  });
}

export function ensureMatrix(matrix: number[][], size: number): number[][] {
  if (matrix.length !== size) {
    const filled = new Array(size).fill(null).map((_, idx) => matrix[idx] ?? new Array(size).fill(0));
    return filled.map((row, rowIdx) =>
      row.length === size ? row : new Array(size).fill(0).map((_, idx) => row[idx] ?? 0)
    );
  }
  return matrix.map((row) => (row.length === size ? row : new Array(size).fill(0).map((_, idx) => row[idx] ?? 0)));
}
