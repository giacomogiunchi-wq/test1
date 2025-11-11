import { describe, expect, it } from 'vitest';
import { applyCapacity, distributeFlow, multiplyMatrixVector, normalizeVector } from '../src/engine/math';

describe('normalizeVector', () => {
  it('normalizes positive numbers', () => {
    expect(normalizeVector([2, 2])).toEqual([0.5, 0.5]);
  });

  it('handles zero sum', () => {
    const result = normalizeVector([0, 0, 0]);
    expect(result.reduce((acc, val) => acc + val, 0)).toBeCloseTo(1);
  });
});

describe('multiplyMatrixVector', () => {
  it('multiplies matrices', () => {
    const matrix = [
      [1, 0],
      [0, 1]
    ];
    expect(multiplyMatrixVector(matrix, [2, 3])).toEqual([2, 3]);
  });
});

describe('applyCapacity', () => {
  it('caps flow and applies losses', () => {
    expect(applyCapacity(120, 100, 0.1)).toBeCloseTo(90);
  });
});

describe('distributeFlow', () => {
  it('distributes respecting split', () => {
    const flows = distributeFlow(100, [0.7, 0.3], 2);
    expect(flows[0]).toBeCloseTo(70);
    expect(flows[1]).toBeCloseTo(30);
  });
});
