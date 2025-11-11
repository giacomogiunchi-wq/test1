import { useMemo } from 'react';
import { useGraphStore } from '../state/graphStore';
import { formatComposition, UNIT_FLOW } from '../lib/units';
import { ensureMatrix, normalize } from '../lib/matrix';

export function Inspector() {
  const {
    nodes,
    selectedNodeId,
    updateNodeData,
    families,
    results,
    removeNode
  } = useGraphStore((state) => ({
    nodes: state.nodes,
    selectedNodeId: state.selectedNodeId,
    updateNodeData: state.updateNodeData,
    families: state.families,
    results: state.results,
    removeNode: state.removeNode
  }));

  const node = useMemo(() => nodes.find((n) => n.id === selectedNodeId), [nodes, selectedNodeId]);
  const lastMeasure = selectedNodeId ? results[selectedNodeId]?.last : undefined;

  if (!node) {
    return (
      <aside className="w-80 border-l border-slate-200 bg-slate-50 p-4 text-sm dark:border-slate-700 dark:bg-slate-800">
        <h2 className="text-lg font-semibold">Inspector</h2>
        <p className="mt-4 text-slate-500">Seleziona un nodo per modificare le proprietà.</p>
      </aside>
    );
  }

  const data = node.data;

  const handleSplitChange = (index: number, value: number) => {
    if (data.type !== 'machine') return;
    const newSplit = [...data.split];
    newSplit[index] = value;
    const total = newSplit.reduce((acc, val) => acc + val, 0);
    const normalized = total === 0 ? newSplit.map(() => 0) : newSplit.map((val) => val / total);
    updateNodeData(node.id, { split: normalized } as any);
  };

  return (
    <aside className="w-80 overflow-y-auto border-l border-slate-200 bg-slate-50 p-4 text-sm dark:border-slate-700 dark:bg-slate-800">
      <div className="flex items-center justify-between">
        <h2 className="text-lg font-semibold">{node.data.type.toUpperCase()}</h2>
        <button
          type="button"
          className="rounded-md border border-red-500 px-2 py-1 text-xs text-red-500 hover:bg-red-500/10"
          onClick={() => {
            if (window.confirm('Eliminare il nodo selezionato?')) {
              removeNode(node.id);
            }
          }}
        >
          Elimina
        </button>
      </div>
      {data.type === 'start' && (
        <div className="mt-4 space-y-3">
          <label className="block">
            <span className="text-slate-500">Portata ({UNIT_FLOW})</span>
            <input
              type="number"
              value={data.portata}
              onChange={(event) => updateNodeData(node.id, { portata: Number(event.target.value) })}
              className="mt-1 w-full rounded-md border border-slate-300 bg-transparent px-2 py-1 dark:border-slate-600"
            />
          </label>
          <div>
            <span className="text-slate-500">Composizione iniziale</span>
            {families.map((family, idx) => (
              <label key={family.id} className="mt-2 block">
                <span className="text-xs text-slate-400">{family.label}</span>
                <input
                  type="number"
                  step="0.01"
                  value={data.composizione[idx] ?? 0}
                  onChange={(event) => {
                    const next = [...data.composizione];
                    next[idx] = Number(event.target.value);
                    updateNodeData(node.id, { composizione: normalize(next) } as any);
                  }}
                  className="mt-1 w-full rounded-md border border-slate-300 bg-transparent px-2 py-1 dark:border-slate-600"
                />
              </label>
            ))}
          </div>
        </div>
      )}
      {data.type === 'machine' && (
        <div className="mt-4 space-y-3">
          <label className="block">
            <span className="text-slate-500">Throughput max ({UNIT_FLOW})</span>
            <input
              type="number"
              value={data.throughputMax}
              onChange={(event) => updateNodeData(node.id, { throughputMax: Number(event.target.value) } as any)}
              className="mt-1 w-full rounded-md border border-slate-300 bg-transparent px-2 py-1 dark:border-slate-600"
            />
          </label>
          <label className="block">
            <span className="text-slate-500">Fattore di perdita</span>
            <input
              type="number"
              min={0}
              max={1}
              step={0.01}
              value={data.lossFactor}
              onChange={(event) => updateNodeData(node.id, { lossFactor: Number(event.target.value) } as any)}
              className="mt-1 w-full rounded-md border border-slate-300 bg-transparent px-2 py-1 dark:border-slate-600"
            />
          </label>
          <div>
            <div className="flex items-center justify-between">
              <span className="text-slate-500">Matrice efficienza</span>
              <button
                type="button"
                className="text-xs text-primary"
                onClick={() =>
                  updateNodeData(
                    node.id,
                    {
                      matrix: ensureMatrix(
                        families.map((_, idx) =>
                          families.map((__, jdx) => (idx === jdx ? 1 : 0))
                        ),
                        families.length
                      )
                    } as any
                  )
                }
              >
                Identità
              </button>
            </div>
            <div className="mt-2 grid grid-cols-3 gap-2 text-xs">
              {ensureMatrix(data.matrix, families.length).map((row, rIdx) =>
                row.map((value, cIdx) => (
                  <input
                    key={`${rIdx}-${cIdx}`}
                    type="number"
                    min={0}
                    max={1}
                    step={0.01}
                    value={value}
                    onChange={(event) => {
                      const matrix = ensureMatrix(data.matrix, families.length).map((r) => [...r]);
                      matrix[rIdx][cIdx] = Number(event.target.value);
                      updateNodeData(node.id, { matrix } as any);
                    }}
                    className="rounded-md border border-slate-300 bg-transparent px-2 py-1 text-xs dark:border-slate-600"
                  />
                ))
              )}
            </div>
          </div>
          <div>
            <span className="text-slate-500">Split uscita (%)</span>
            {data.split.map((value, idx) => (
              <label key={idx} className="mt-1 flex items-center gap-2">
                <span className="w-12 text-xs text-slate-400">Out {idx + 1}</span>
                <input
                  type="range"
                  min={0}
                  max={100}
                  value={Math.round(value * 100)}
                  onChange={(event) => handleSplitChange(idx, Number(event.target.value) / 100)}
                  className="flex-1"
                />
                <span className="w-12 text-right text-xs">{(value * 100).toFixed(0)}%</span>
              </label>
            ))}
          </div>
        </div>
      )}
      {lastMeasure && (
        <div className="mt-6 rounded-md border border-slate-200 bg-white p-3 text-xs dark:border-slate-700 dark:bg-slate-900">
          <div className="font-semibold">Misura ultimo step</div>
          <div className="mt-1">Step: {lastMeasure.t}</div>
          <div className="mt-1">Portata: {lastMeasure.Q.toFixed(1)} {UNIT_FLOW}</div>
          <div className="mt-1">Composizione: {formatComposition(lastMeasure.c)}</div>
        </div>
      )}
    </aside>
  );
}
