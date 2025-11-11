import { formatComposition, formatFlow } from '../lib/units';

export type MeterCardProps = {
  title: string;
  flow: number;
  composition: number[];
};

export function MeterCard({ title, flow, composition }: MeterCardProps) {
  return (
    <div className="rounded-md border border-slate-200 bg-white px-3 py-2 text-xs shadow dark:border-slate-700 dark:bg-slate-900">
      <div className="font-semibold">{title}</div>
      <div className="mt-1 text-slate-500">Portata: {formatFlow(flow)}</div>
      <div className="mt-1 text-slate-500">Composizione: {formatComposition(composition)}</div>
    </div>
  );
}
