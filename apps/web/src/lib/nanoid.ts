let counter = 0;
export function nanoid(prefix = 'node') {
  counter += 1;
  return `${prefix}-${counter}`;
}
